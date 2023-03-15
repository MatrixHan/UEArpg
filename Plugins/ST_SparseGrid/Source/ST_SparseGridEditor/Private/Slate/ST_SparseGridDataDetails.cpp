// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridDataDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailGroup.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"

#include "Editor.h"

// Extras
#include "ST_SparseGridData.h"

#define LOCTEXT_NAMESPACE "NSDataDetails"

TSharedRef<IDetailCustomization> FST_SparseGridDataDetails::MakeInstance()
{
	return MakeShareable(new FST_SparseGridDataDetails);
}

void FST_SparseGridDataDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Cache the Object
	TArray<TWeakObjectPtr<UObject>> ObjectList;
	DetailBuilder.GetObjectsBeingCustomized(ObjectList);

	check(ObjectList.Num() == 1);
	
	UST_SparseGridData* GridData = Cast<UST_SparseGridData>(ObjectList[0].Get());
	check(GridData);

	Cached_DataObject = GridData;

	// Unfortunately, it's not possible to just append properties or modify the widget in place
	// This means we have to iterate all properties and add them manually so we can set the order
	CustomizeGridProperties(DetailBuilder);
	CustomizeMemoryManagement(DetailBuilder);
	CustomizeVisualization(DetailBuilder);
}

//////////////////////////////////
///// Category Customization /////
//////////////////////////////////

void FST_SparseGridDataDetails::CustomizeGridProperties(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category_GridProperties = DetailBuilder.EditCategory(TEXT("Grid Properties"));

	TSharedRef<IPropertyHandle> PropHandle_GridOrigin = DetailBuilder.GetProperty("GridOrigin");
	TSharedRef<IPropertyHandle> PropHandle_NumCellsX = DetailBuilder.GetProperty("NumCellsX");
	TSharedRef<IPropertyHandle> PropHandle_NumCellsY = DetailBuilder.GetProperty("NumCellsY");
	TSharedRef<IPropertyHandle> PropHandle_CellSize = DetailBuilder.GetProperty("CellSize");

	check(PropHandle_GridOrigin->IsValidHandle());
	check(PropHandle_NumCellsX->IsValidHandle());
	check(PropHandle_NumCellsY->IsValidHandle());
	check(PropHandle_CellSize->IsValidHandle());

	TOptional<int32> UnsetLimit = TOptional<int32>();
	TOptional<int32> CellsMin = TOptional<int32>(PropHandle_NumCellsX->GetIntMetaData(TEXT("ClampMin")));
	TOptional<int32> CellsMax = TOptional<int32>(PropHandle_NumCellsX->GetIntMetaData(TEXT("ClampMax")));

	PropHandle_NumCellsX->MarkHiddenByCustomization();
	PropHandle_NumCellsY->MarkHiddenByCustomization();
	PropHandle_NumCellsY->MarkResetToDefaultCustomized();
	PropHandle_NumCellsX->MarkResetToDefaultCustomized();
	PropHandle_GridOrigin->MarkResetToDefaultCustomized();

	// Grid Origin Transform Box
	Category_GridProperties.AddProperty(PropHandle_GridOrigin).CustomWidget(false)
	.NameContent()
	[
		PropHandle_GridOrigin->CreatePropertyNameWidget()
	]
	.ValueContent().HAlign(HAlign_Left).VAlign(VAlign_Center).MinDesiredWidth(125.f * 2.f).MaxDesiredWidth(125.f * 2.f)
	[
		// Custom Box
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.0f, 1.0f, 2.0f, 1.0f)
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Value(this, &FST_SparseGridDataDetails::GetGridOriginX)
			.OnValueChanged(this, &FST_SparseGridDataDetails::SetGridOriginX, ETextCommit::Default)
			.OnValueCommitted(this, &FST_SparseGridDataDetails::SetGridOriginX)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.LabelPadding(0)
 			.MinValue(UnsetLimit)
 			.MaxValue(UnsetLimit)
			.MinSliderValue(UnsetLimit)
			.MaxSliderValue(UnsetLimit)
			.Label()
			[
				SNumericEntryBox<int32>::BuildLabel(LOCTEXT("X", "X"), FSlateColor(FLinearColor::White), SNumericEntryBox<int32>::RedLabelBackgroundColor)
			]
		]
		+SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.0f, 1.0f, 2.0f, 1.0f)
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Value(this, &FST_SparseGridDataDetails::GetGridOriginY)
			.OnValueChanged(this, &FST_SparseGridDataDetails::SetGridOriginY, ETextCommit::Default)
			.OnValueCommitted(this, &FST_SparseGridDataDetails::SetGridOriginY)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.LabelPadding(0)
			.MinValue(UnsetLimit)
			.MaxValue(UnsetLimit)
			.MinSliderValue(UnsetLimit)
			.MaxSliderValue(UnsetLimit)
			.Label()
			[
				SNumericEntryBox<int32>::BuildLabel(LOCTEXT("Y", "Y"), FSlateColor(FLinearColor::White), SNumericEntryBox<int32>::GreenLabelBackgroundColor)
			]
		]
	];

	Category_GridProperties.AddCustomRow(LOCTEXT("NumCellsFilter", "Num Cells"), false)
	.NameContent()
	[
		PropHandle_NumCellsX->CreatePropertyNameWidget(
			LOCTEXT("NumCellsName", "Number of Cells"),
			LOCTEXT("NumCellsTooltip", "* Number of Cells in X and Y directions.\r\n\r\n* Values should be adjusted so that the cells fully encompass the playable area of the world.\r\n* Objects outside of the grid will be clamped to boundary cells."))
	]
	.ValueContent().HAlign(HAlign_Left).VAlign(VAlign_Center).MinDesiredWidth(125.f * 2.f).MaxDesiredWidth(125.f * 2.f)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.0f, 1.0f, 2.0f, 1.0f)
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Value(this, &FST_SparseGridDataDetails::GetNumCellsX)
			.OnValueChanged(this, &FST_SparseGridDataDetails::SetNumCellsX, ETextCommit::Default)
			.OnValueCommitted(this, &FST_SparseGridDataDetails::SetNumCellsX)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.LabelPadding(0)
			.MinValue(CellsMin)
			.MaxValue(CellsMax)
			.MinSliderValue(CellsMin)
			.MaxSliderValue(CellsMax)
			.Label()
			[
				SNumericEntryBox<int32>::BuildLabel(LOCTEXT("X", "X"), FSlateColor(FLinearColor::White), SNumericEntryBox<int32>::RedLabelBackgroundColor)
			]
		]
		+SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.0f, 1.0f, 2.0f, 1.0f)
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Value(this, &FST_SparseGridDataDetails::GetNumCellsY)
			.OnValueChanged(this, &FST_SparseGridDataDetails::SetNumCellsY, ETextCommit::Default)
			.OnValueCommitted(this, &FST_SparseGridDataDetails::SetNumCellsY)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.LabelPadding(0)
			.MinValue(CellsMin)
			.MaxValue(CellsMax)
			.MinSliderValue(CellsMin)
			.MaxSliderValue(CellsMax)
			.Label()
			[
				SNumericEntryBox<int32>::BuildLabel(LOCTEXT("Y", "Y"), FSlateColor(FLinearColor::White), SNumericEntryBox<int32>::GreenLabelBackgroundColor)
			]
		]
	];	
}

void FST_SparseGridDataDetails::CustomizeMemoryManagement(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category_MemoryManagement = DetailBuilder.EditCategory(TEXT("Memory Management"));

	TSharedRef<IPropertyHandle> PropHandle_ListAlloc = DetailBuilder.GetProperty("RegisterAllocSize");
	TSharedRef<IPropertyHandle> PropHandle_ListShrink = DetailBuilder.GetProperty("RegisterAllocShrinkMultiplier");
	TSharedRef<IPropertyHandle> PropHandle_CellAlloc = DetailBuilder.GetProperty("CellAllocSize");
	TSharedRef<IPropertyHandle> PropHandle_CellShrink = DetailBuilder.GetProperty("CellAllocShrinkMultiplier");

	check(PropHandle_ListAlloc->IsValidHandle());
	check(PropHandle_ListShrink->IsValidHandle());
	check(PropHandle_CellAlloc->IsValidHandle());
	check(PropHandle_CellShrink->IsValidHandle());

	DetailBuilder.AddPropertyToCategory(PropHandle_ListAlloc);
	DetailBuilder.AddPropertyToCategory(PropHandle_CellAlloc);

	PropHandle_CellShrink->MarkHiddenByCustomization();
	PropHandle_ListShrink->MarkHiddenByCustomization();

	// Show Block Allocation Tooltip
	Category_MemoryManagement.AddCustomRow(LOCTEXT("ListShrinkFilter", ""), true)
	.NameContent().VAlign(VAlign_Top)
	[
		PropHandle_ListShrink->CreatePropertyNameWidget()
	]
	.ValueContent().HAlign(HAlign_Left).MinDesiredWidth(125.f + 150.f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).MinDesiredWidth(125.f).MaxDesiredWidth(125.f)
			[
				PropHandle_ListShrink->CreatePropertyValueWidget()
			]
		]
		+SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(this, &FST_SparseGridDataDetails::GetListShrinkTooltipText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Justification(ETextJustify::Left)
			.IsEnabled(false)
			.Visibility(EVisibility::SelfHitTestInvisible)
		]
	];

	Category_MemoryManagement.AddCustomRow(LOCTEXT("CellShrinkFilter", ""), true)
	.NameContent().VAlign(VAlign_Top)
	[
		PropHandle_CellShrink->CreatePropertyNameWidget()
	]
	.ValueContent().HAlign(HAlign_Left).MinDesiredWidth(125.f + 150.f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).MinDesiredWidth(125.f).MaxDesiredWidth(125.f)
			[
				PropHandle_CellShrink->CreatePropertyValueWidget()
			]
		]
		+SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(this, &FST_SparseGridDataDetails::GetCellShrinkTooltipText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Justification(ETextJustify::Left)
			.IsEnabled(false)
			.Visibility(EVisibility::SelfHitTestInvisible)
		]
	];
}

void FST_SparseGridDataDetails::CustomizeVisualization(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> PropHandle_DrawAs = DetailBuilder.GetProperty("DrawAs");
	TSharedRef<IPropertyHandle> PropHandle_DrawAltitude = DetailBuilder.GetProperty("DrawAltitude");
	TSharedRef<IPropertyHandle> PropHandle_BoxExtent = DetailBuilder.GetProperty("BoxExtent");
	TSharedRef<IPropertyHandle> PropHandle_GridColour = DetailBuilder.GetProperty("GridColour");

	check(PropHandle_DrawAs->IsValidHandle());
	check(PropHandle_DrawAltitude->IsValidHandle());
	check(PropHandle_BoxExtent->IsValidHandle());
	check(PropHandle_GridColour->IsValidHandle());

	// Hide Box Extents in certain conditions
	TAttribute<EVisibility> CustomExtentVisibility = TAttribute<EVisibility>(this, &FST_SparseGridDataDetails::GetBoxExtentVisibility);

	// Add Properties in a sensible order
	DetailBuilder.AddPropertyToCategory(PropHandle_DrawAs);
	DetailBuilder.AddPropertyToCategory(PropHandle_DrawAltitude);
	DetailBuilder.AddPropertyToCategory(PropHandle_BoxExtent).Visibility(CustomExtentVisibility);
	DetailBuilder.AddPropertyToCategory(PropHandle_GridColour);
}

/////////////////////////////
///// Property Bindings /////
/////////////////////////////

TOptional<int32> FST_SparseGridDataDetails::GetGridOriginX() const
{
	return Cached_DataObject.IsValid() ? Cached_DataObject.Get()->GetGridOrigin().X : 0;
}

TOptional<int32> FST_SparseGridDataDetails::GetGridOriginY() const
{
	return Cached_DataObject.IsValid() ? Cached_DataObject.Get()->GetGridOrigin().Y : 0;
}

TOptional<int32> FST_SparseGridDataDetails::GetNumCellsX() const
{
	return Cached_DataObject.IsValid() ? Cached_DataObject.Get()->GetNumCellsX() : 0;
}

TOptional<int32> FST_SparseGridDataDetails::GetNumCellsY() const
{
	return Cached_DataObject.IsValid() ? Cached_DataObject.Get()->GetNumCellsY() : 0;
}

EVisibility FST_SparseGridDataDetails::GetBoxExtentVisibility() const
{
	return (Cached_DataObject.IsValid() && Cached_DataObject.Get()->GetDrawType() == EST_SGVisualizerType::DDT_BoxGrid) ? EVisibility::Visible : EVisibility::Collapsed;
}

void FST_SparseGridDataDetails::SetGridOriginX(int32 InOriginX, ETextCommit::Type CommitInfo)
{
	if (Cached_DataObject.IsValid())
	{
		Cached_DataObject.Get()->SetGridOrigin(FST_GridRef2D(InOriginX, Cached_DataObject.Get()->GetGridOrigin().Y));
		GEditor->RedrawLevelEditingViewports(false);
	}
}

void FST_SparseGridDataDetails::SetGridOriginY(int32 InOriginY, ETextCommit::Type CommitInfo)
{
	if (Cached_DataObject.IsValid())
	{
		Cached_DataObject.Get()->SetGridOrigin(FST_GridRef2D(Cached_DataObject.Get()->GetGridOrigin().X, InOriginY));
		GEditor->RedrawLevelEditingViewports(false);
	}
}

void FST_SparseGridDataDetails::SetNumCellsX(int32 InNumCellsX, ETextCommit::Type CommitInfo)
{
	if (Cached_DataObject.IsValid())
	{
		Cached_DataObject.Get()->SetNumCellsX(InNumCellsX);
		GEditor->RedrawLevelEditingViewports(false);
	}
}

void FST_SparseGridDataDetails::SetNumCellsY(int32 InNumCellsY, ETextCommit::Type CommitInfo)
{
	if (Cached_DataObject.IsValid())
	{
		Cached_DataObject.Get()->SetNumCellsY(InNumCellsY);
		GEditor->RedrawLevelEditingViewports(false);
	}
}

FText FST_SparseGridDataDetails::GetListShrinkTooltipText() const
{
	if (Cached_DataObject.IsValid())
	{
		const int32 SValue = Cached_DataObject->GetRegisterAllocShrinkMultiplier();
		const int32 AValue = Cached_DataObject->GetRegisterAllocSize();

		if (SValue == INDEX_NONE)
		{
			return LOCTEXT("NeverShrink", "Never Shrink");
		}
		else if (SValue == 0)
		{
			return FText::Format(LOCTEXT("ListAlwaysShrink", "Shrink to nearest {0}"), FText::AsNumber(AValue));
		}
		else
		{
			return FText::Format(LOCTEXT("ListMinShrink", "Shrink to nearest {0}, retain {1} once allocated"), FText::AsNumber(AValue), FText::AsNumber(AValue * SValue));
		}
	}

	return FText();
}

FText FST_SparseGridDataDetails::GetCellShrinkTooltipText() const
{
	if (Cached_DataObject.IsValid())
	{
		const int32 SValue = Cached_DataObject->GetCellAllocShrinkMultiplier();
		const int32 AValue = Cached_DataObject->GetCellAllocSize();

		if (SValue == INDEX_NONE)
		{
			return LOCTEXT("NeverShrink", "Never Shrink");
		}
		else if (SValue == 0)
		{
			return FText::Format(LOCTEXT("CellAlwaysShrink", "Shrink to nearest {0}"), FText::AsNumber(AValue));
		}
		else
		{
			return FText::Format(LOCTEXT("CellMinShrink", "Shrink to nearest {0}, retain {1} once allocated"), FText::AsNumber(AValue), FText::AsNumber(AValue * SValue));
		}
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE