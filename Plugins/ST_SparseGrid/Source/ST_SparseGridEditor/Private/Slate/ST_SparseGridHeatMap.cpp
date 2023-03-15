// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridHeatMap.h"
#include "ST_SparseGridManager.h"
#include "ST_SparseGridData.h"

// IWYU
#include "Engine/Texture2D.h"
#include "RenderingThread.h"
#include "RenderUtils.h"
#include "Rendering/DrawElements.h"

////////////////////////
///// Construction /////
////////////////////////

SST_SparseGridHeatMap::SST_SparseGridHeatMap()
	: Image(FSlateBrush())
	, DataBuffer(nullptr)
	, GridManager(nullptr)
	, ColdColour(FLinearColor::Green)
	, HotColour(FLinearColor::Red)
	, HotThreshold(16.f)
{
	SetCanTick(true);
	bCanSupportFocus = false;
}

void SST_SparseGridHeatMap::Construct(const FArguments& InArgs)
{
	HotColour = InArgs._HotColour;
	ColdColour = InArgs._ColdColour;
	HotThreshold = InArgs._HotThreshold;
}

//////////////////////////////////
///// Data Buffer Management /////
//////////////////////////////////

void SST_SparseGridHeatMap::SetGridManager(UST_SparseGridManager* InGrid, const FName InGridID)
{
	GridManager = InGrid;
	GridID = InGridID;

	CheckBuffer();
}

void SST_SparseGridHeatMap::CreateBuffer()
{
	if (GridManager.IsValid() && GridManager->GetGridConfig())
	{
		if (!DataBuffer)
		{
			DataBuffer = UTexture2D::CreateTransient(GridManager->GetGridConfig()->GetNumCellsX(), GridManager->GetGridConfig()->GetNumCellsY());
			checkf(DataBuffer != nullptr, TEXT("Unable to Create Data Buffer"));

			DataBuffer->Filter = TextureFilter::TF_Nearest;
			DataBuffer->SRGB = true;	// Must be SRGB, or exported textures have incorrect values
			DataBuffer->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
#if WITH_EDITORONLY_DATA
			DataBuffer->CompressionNoAlpha = false;
			DataBuffer->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif // WITH_EDITORONLY_DATA
			DataBuffer->AddressX = TextureAddress::TA_Clamp;
			DataBuffer->AddressY = TextureAddress::TA_Clamp;

			// Creates the GPU Resource
			DataBuffer->UpdateResource();
			DataBuffer->RefreshSamplerStates();
		}

		checkf(DataBuffer && DataBuffer->IsValidLowLevel(), TEXT("Unable to Verify Data Buffer"));

		// Cache Size
		CachedSizeBytes = CalculateImageBytes(DataBuffer->GetSizeX(), DataBuffer->GetSizeY(), 0, DataBuffer->GetPixelFormat());

		// Create Brush
		FSlateBrush NewBrush;
		NewBrush.SetResourceObject(DataBuffer);
		NewBrush.ImageSize = FVector2D(GridManager->GetGridConfig()->GetNumCellsX(), GridManager->GetGridConfig()->GetNumCellsY());
		NewBrush.DrawAs = ESlateBrushDrawType::Image;

		Image = NewBrush;
	}
}

void SST_SparseGridHeatMap::DestroyBuffer()
{
	Image = FSlateBrush();
	if (DataBuffer && DataBuffer->IsValidLowLevel())
	{
		// Not had a crash here yet, but could we cause one if this gets destroyed before render thread is done?
		FlushRenderingCommands();

		// Allow buffer to be safely cleared by GC
		DataBuffer->ConditionalBeginDestroy();
		DataBuffer = nullptr;
	}

	CachedSizeBytes = 0;
}

void SST_SparseGridHeatMap::CheckBuffer()
{
	bool bBufferValid = false;
	if (GridManager.IsValid() && GridManager->GetGridConfig() && DataBuffer)
	{
		const int32 DesiredWidth = GridManager->GetGridConfig()->GetNumCellsX();
		const int32 DesiredHeight = GridManager->GetGridConfig()->GetNumCellsY();
		if ((DesiredWidth >= 1 && DesiredHeight >= 1) && (int32)DataBuffer->GetSizeX() == DesiredWidth && (int32)DataBuffer->GetSizeY() == DesiredHeight)
		{
			bBufferValid = true;
		}
	}

	if (!bBufferValid)
	{
		DestroyBuffer();
		CreateBuffer();
	}
}

void SST_SparseGridHeatMap::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(DataBuffer);
}

/////////////////
///// Paint /////
/////////////////

void SST_SparseGridHeatMap::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Super Equivalent
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Ensure the buffer is still valid for the world and play area size
	CheckBuffer();

	// Draw Data Buffer
	if (GridManager.IsValid()
		&& GridManager->GetGridConfig()
		&& DataBuffer != nullptr)
	{
		const uint32 Width = DataBuffer->GetSizeX();
		const uint32 Height = DataBuffer->GetSizeY();

		// TODO: Get Populations so that Texture Axes match world axes (Y is currently flipped as texture origin is top-left)
		if (GridManager->GetGridPopulationData(GridID, PixelColours))
		{
			// If we don't have exactly the right amount of data, the render thread command will crash
			checkf(PixelColours.Num() == (Width * Height), TEXT("Not enough population data for Data Buffer!"));

			// Convert Numeric Value into FColor
			for (uint32& Count : PixelColours)
			{
				const float Alpha = FMath::Clamp((float)Count / FMath::Max<float>(HotThreshold.Get(), 1.f), 0.f, 1.f);
				const FColor CellColour = FLinearColor::LerpUsingHSV(ColdColour.Get(), HotColour.Get(), Alpha).ToFColor(true);

				// Pack in reverse order to texture format
				// E.g PF_B8GBR8A8 is ARGB, PF_R8G8B8A8 is ABGR
				Count = CellColour.ToPackedARGB();
			}

			// Update texture on Render Thread
			// NOTE: Texture will be padded to power-of-two, so when reading back, we need to factor that in
			ENQUEUE_RENDER_COMMAND(FST_SGHeatMapGrid)(
				[Data = PixelColours, Size = CachedSizeBytes, Tex = DataBuffer->Resource->TextureRHI](FRHICommandListImmediate& RHICmdList)
 				{
					if (ensure(Tex.IsValid()))
					{
						uint32 DestStride;
						uint32* DestBuffer = static_cast<uint32*>(RHILockTexture2D(Tex->GetTexture2D(), 0, RLM_WriteOnly, DestStride, false, false));
						FMemory::Memcpy(DestBuffer, Data.GetData(), Size);
						RHIUnlockTexture2D(Tex->GetTexture2D(), 0, false, false);
					}
 				});
		}
	}
}

int32 SST_SparseGridHeatMap::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Image.DrawAs != ESlateBrushDrawType::NoDrawType)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &Image, ESlateDrawEffect::None, FLinearColor(InWidgetStyle.GetColorAndOpacityTint()));
	}

	return LayerId;
}

FVector2D SST_SparseGridHeatMap::ComputeDesiredSize(float) const
{
	return Image.ImageSize;
}

//////////////////
///// Design /////
//////////////////

void SST_SparseGridHeatMap::SetHotColour(const FLinearColor& InHotColour)
{
	if (HotColour.IsBound() || HotColour.Get() != InHotColour)
	{
		HotColour = InHotColour;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SST_SparseGridHeatMap::SetColdColour(const FLinearColor& InColdColour)
{
	if (ColdColour.IsBound() || ColdColour.Get() != InColdColour)
	{
		ColdColour = InColdColour;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SST_SparseGridHeatMap::SetHotThreshold(const float InHotThreshold)
{
	if (HotThreshold.IsBound() || HotThreshold.Get() != InHotThreshold)
	{
		HotThreshold = InHotThreshold;
		Invalidate(EInvalidateWidget::Layout);
	}
}