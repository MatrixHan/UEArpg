// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridHeatmapTab.h"
#include "ST_SparseGridHeatMap.h"
#include "ST_SparseGridManager.h"
#include "ST_SparseGridData.h"
#include "ST_SparseGridHeatmapProxy.h"
#include "ST_SparseGridEditorModule.h"

// Property Editor
#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"

// Slate
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Editor/KismetWidgets/Public/SLevelOfDetailBranchNode.h"
#include "SlateOptMacros.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Multibox/MultiBoxBuilder.h"
#include "Widgets/Input/STextComboBox.h"

// Editor
#include "Editor.h" // Ugh

// Image Saving
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "NSSparseGridTab"

///////////////////
///// Statics /////
///////////////////

TSharedRef<SDockTab> SST_SparseGridHeatmapTab::CreateTab(const FSpawnTabArgs& InArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("TabTitle", "Grid Heatmap"))
		[
			SNew(SST_SparseGridHeatmapTab)
		];
}

////////////////////////
///// Construction /////
////////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SST_SparseGridHeatmapTab::Construct(const FArguments& InArgs)
{
	GenerateManagerOptions(false);
	CurrentDebuggingManager = nullptr;

	// Refresh Functions
	OnPieStartedDelegateHandle = FEditorDelegates::BeginPIE.AddSP(this, &SST_SparseGridHeatmapTab::OnPIEStarted);
	OnPieEndedDelegateHandle = FEditorDelegates::EndPIE.AddSP(this, &SST_SparseGridHeatmapTab::OnPIEEnded);
	OnMapChangedDelegateHandle = FEditorDelegates::MapChange.AddSP(this, &SST_SparseGridHeatmapTab::OnMapChanged);
	OnNewCurrentLevelDelegateHandle = FEditorDelegates::NewCurrentLevel.AddSP(this, &SST_SparseGridHeatmapTab::OnNewCurrentLevel);

	// Create Proxy Details
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs = FDetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);

	Proxy_DetailsPanel = PropertyModule.CreateDetailView(DetailsViewArgs);
	Proxy_DetailsPanel->SetObject(EditorProxy, true);

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().AutoHeight().Padding(2.f).HAlign(HAlign_Fill).VAlign(VAlign_Center)
		[
			CreateToolbar()
		]
		+SVerticalBox::Slot().FillHeight(1.f).Padding(2.f).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
 			SNew(SBorder)
			.Padding(4.f)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SOverlay)
				+SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SNew(SScaleBox)
					.StretchDirection(EStretchDirection::Both)
					.Stretch(EStretch::ScaleToFit)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						CreateHeatmapWidget()
					]
				]
				+SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom)
				[
					CreateDiagnostics()
				]
			]
		]
		+SVerticalBox::Slot().AutoHeight().Padding(2.f).VAlign(VAlign_Top).HAlign(HAlign_Fill)
		[
			Proxy_DetailsPanel.ToSharedRef()
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SST_SparseGridHeatmapTab::SST_SparseGridHeatmapTab()
	: SCompoundWidget()
{
	EditorProxy = NewObject<UST_SparseGridHeatmapProxy>(GetTransientPackage(), NAME_None, RF_Transactional);
	bShowingDiagnostics = false;
}

SST_SparseGridHeatmapTab::~SST_SparseGridHeatmapTab()
{
	FEditorDelegates::BeginPIE.Remove(OnPieStartedDelegateHandle);
	FEditorDelegates::EndPIE.Remove(OnPieEndedDelegateHandle);
	FEditorDelegates::MapChange.Remove(OnMapChangedDelegateHandle);
	FEditorDelegates::MapChange.Remove(OnNewCurrentLevelDelegateHandle);
}

///////////////////////////////
///// Sub-Widget Creation /////
///////////////////////////////

TSharedRef<SWidget> SST_SparseGridHeatmapTab::CreateHeatmapWidget()
{
	if (!HeatmapWidget.IsValid())
	{
		checkf(EditorProxy != nullptr, TEXT("Invalid Editor Proxy!"));

		// Bind Info to the Editor Proxy Object
		HeatmapWidget = SNew(SST_SparseGridHeatMap)
			.HotColour(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateUObject(EditorProxy, &UST_SparseGridHeatmapProxy::GetHotColour)))
			.ColdColour(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateUObject(EditorProxy, &UST_SparseGridHeatmapProxy::GetColdColour)))
			.HotThreshold(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(EditorProxy, &UST_SparseGridHeatmapProxy::GetHotThreshold)));
	}

	return HeatmapWidget.ToSharedRef();
}

TSharedRef<SWidget> SST_SparseGridHeatmapTab::CreateToolbar()
{
	FToolBarBuilder ReturnToolbar = FToolBarBuilder(NULL, FMultiBoxCustomization::None);

	// Add 'Save To Disk' Button
	ReturnToolbar.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SST_SparseGridHeatmapTab::ExportHeatMap), FCanExecuteAction::CreateSP(this, &SST_SparseGridHeatmapTab::HasManagerSelected)),
		NAME_None,
		LOCTEXT("ExportMapLabel", "Export"),
		LOCTEXT("ExportMapTooltip", "Exports the current heat map frame to disk."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "AssetEditor.SaveAsset"));

	// Add 'Diagnostics' Button
	ReturnToolbar.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SST_SparseGridHeatmapTab::ToggleDiagnostics), FCanExecuteAction::CreateSP(this, &SST_SparseGridHeatmapTab::HasManagerSelected), FIsActionChecked::CreateSP(this, &SST_SparseGridHeatmapTab::AreDiagnosticsVisible)),
		NAME_None,
		LOCTEXT("DiagnosticsLabel", "Diagnostics"),
		LOCTEXT("DiagnosticsTooltip", "Displays additional diagnostic information about the grid"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "MaterialEditor.ToggleMaterialStats"));

	ReturnToolbar.AddSeparator();

	// Create Debug Grid Drop-Down
	ManagersComboBox = SNew(STextComboBox)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Light")
		.OptionsSource(&ManagerOptions)
		.InitiallySelectedItem(GetInitialManagerOption())
		.OnComboBoxOpening(this, &SST_SparseGridHeatmapTab::GenerateManagerOptions, true)
		.OnSelectionChanged(this, &SST_SparseGridHeatmapTab::OnManagerSelected);
	ManagersComboBox->SetEnabled(TAttribute<bool>(this, &SST_SparseGridHeatmapTab::IsManagerComboBoxEnabled));

	// Create Debug Grid Names Combo-Box
	GridsComboBox = SNew(STextComboBox)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Light")
		.OptionsSource(&GridOptions)
		.InitiallySelectedItem(GetInitialGridOption())
		.OnComboBoxOpening(this, &SST_SparseGridHeatmapTab::GenerateGridOptions)
		.OnSelectionChanged(this, &SST_SparseGridHeatmapTab::OnGridSelected);
	GridsComboBox->SetEnabled(TAttribute<bool>(this, &SST_SparseGridHeatmapTab::IsGridComboBoxEnabled));

	TSharedRef<SWidget> DetailSelector = 
		SNew(SLevelOfDetailBranchNode)
		.UseLowDetailSlot(FMultiBoxSettings::UseSmallToolBarIcons)
		.LowDetail()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(2.f)
			[
				ManagersComboBox.ToSharedRef()
			]
			+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(2.f)
			[
				GridsComboBox.ToSharedRef()
			]
		]
		.HighDetail()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Center).HAlign(HAlign_Fill).Padding(2.f)
			[
				ManagersComboBox.ToSharedRef()
			]
			+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Center).HAlign(HAlign_Fill).Padding(2.f)
			[
				GridsComboBox.ToSharedRef()
			]
		];

	ReturnToolbar.AddWidget(DetailSelector);

	return ReturnToolbar.MakeWidget();
}

TSharedRef<SWidget> SST_SparseGridHeatmapTab::CreateDiagnostics()
{
	// Build Widget
	return SNew(SBorder)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Left)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.75f))
		.Visibility(this, &SST_SparseGridHeatmapTab::GetDiagnosticsVisibility)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(2.f).AutoHeight()
			[
				// Total Object Counter
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).AutoWidth()
				[
					SNew(STextBlock).Text(LOCTEXT("TotalLabel", "Registered Objects")).MinDesiredWidth(120.f)
				]
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).FillWidth(1.f)
				[
					SNew(STextBlock).Text(this, &SST_SparseGridHeatmapTab::Diagnostics_GetTotalObjectsText).MinDesiredWidth(250.f)
				]
			]
			+SVerticalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(2.f).AutoHeight()
			[
				// Cell Memory Use Counter
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).AutoWidth()
				[
					SNew(STextBlock).Text(LOCTEXT("MemoryLabel", "Cells")).MinDesiredWidth(120.f)
				]
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f)).HAlign(HAlign_Left).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(this, &SST_SparseGridHeatmapTab::Diagnostics_GetCellMemoryText).MinDesiredWidth(250.f)
					]
					+SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f)).HAlign(HAlign_Fill).VAlign(VAlign_Center)
					[
						SNew(SProgressBar).Percent(this, &SST_SparseGridHeatmapTab::Diagnostics_GetCellMemoryRatio)
					]
				]
			]
			+SVerticalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(2.f).AutoHeight()
			[
				// Total Memory Use Counter
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).AutoWidth()
				[
					SNew(STextBlock).Text(LOCTEXT("TotalMemoryLabel", "Register")).MinDesiredWidth(120.f)
				]
				+SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f)).HAlign(HAlign_Left).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(this, &SST_SparseGridHeatmapTab::Diagnostics_GetTotalMemoryText).MinDesiredWidth(250.f)
					]
					+SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f)).HAlign(HAlign_Fill).VAlign(VAlign_Center)
					[
						SNew(SProgressBar).Percent(this, &SST_SparseGridHeatmapTab::Diagnostics_GetTotalMemoryRatio)
					]
				]
			]
		];
}

/////////////////////////////
///// Option Generation /////
/////////////////////////////

void SST_SparseGridHeatmapTab::GenerateManagerOptions(bool bRestoreSelection)
{
	// Store Old Selection
	TSharedPtr<FString> OldSelection;
	if (bRestoreSelection && ManagersComboBox.IsValid())
	{
		OldSelection = ManagersComboBox->GetSelectedItem();
	}

	// Empty the lists of grid objects, add a "none selected" option
	Managers.Empty();
	ManagerOptions.Empty();

	Managers.Add(nullptr);
	ManagerOptions.Add(MakeShareable(new FString(LOCTEXT("NoManager", "No Manager Selected").ToString())));

	// Object Iterator on active Grids in any given world
	for (TObjectIterator<UST_SparseGridManager> ManagerItr; ManagerItr; ++ManagerItr)
	{
		UST_SparseGridManager* Manager = *ManagerItr;

		if (!Manager->HasAnyFlags(RF_ClassDefaultObject) && !Manager->IsPendingKill())
		{
			// Get the Manager World
			// Skip if it's not a PIE world
			const FString ManagerName = GetNameSafe(Manager);
			const UWorld* ManagerWorld = Manager->GetWorld();
			if (ManagerName.IsEmpty() || (!ManagerWorld || ManagerWorld->WorldType != EWorldType::PIE))
			{
				continue;
			}

			const ENetMode WorldNM = ManagerWorld->GetNetMode();
			FString WorldName = TEXT("");

			// Get Netmode name for easier identification
			switch (WorldNM)
			{
				case ENetMode::NM_Standalone:
					WorldName = LOCTEXT("DebugWorldStandalone", "Standalone").ToString();
					break;
				case ENetMode::NM_ListenServer:
					WorldName = LOCTEXT("DebugWorldListen", "Listen Server").ToString();
					break;
				case ENetMode::NM_DedicatedServer:
					WorldName = LOCTEXT("DebugWorldDedicated", "Dedicated Server").ToString();
					break;
				case ENetMode::NM_Client:
					if (FWorldContext* PieContext = GEngine->GetWorldContextFromWorld(ManagerWorld))
					{
						WorldName = FString::Printf(TEXT("%s %d"), *LOCTEXT("DebugWorldClient", "Client").ToString(), PieContext->PIEInstance - 1);
					}
					break;
				default:
					break;
			}

			if (!WorldName.IsEmpty())
			{
				Managers.Add(Manager);
				ManagerOptions.Add(MakeShareable(new FString(ManagerName + TEXT(" (") + WorldName + TEXT(")"))));
			}
		}
	}

	if (bRestoreSelection && ManagersComboBox.IsValid())
	{
		bool bMatchFound = false;
		for (int32 GridIdx = 0; GridIdx < ManagerOptions.Num(); GridIdx++)
		{
			if (*ManagerOptions[GridIdx] == *OldSelection)
			{
				ManagersComboBox->SetSelectedItem(ManagerOptions[GridIdx]);
				bMatchFound = true;
				break;
			}
		}

		if (!bMatchFound)
		{
			ManagersComboBox->SetSelectedItem(ManagerOptions[0]);
		}
	}

	// Finally, ensure we have a valid selection
	if (ManagersComboBox.IsValid())
	{
		TSharedPtr<FString> CurrentSelection = ManagersComboBox->GetSelectedItem();
		if (ManagerOptions.Find(CurrentSelection) == INDEX_NONE)
		{
			if (ManagerOptions.Num() > 0)
			{
				ManagersComboBox->SetSelectedItem(ManagerOptions[0]);
			}
			else
			{
				ManagersComboBox->ClearSelection();
			}
		}
	}
}

void SST_SparseGridHeatmapTab::GenerateGridOptions()
{
	// Store Previous Selection (we always attempt to restore it)
	TSharedPtr<FString> OldSelection;
	if (GridsComboBox.IsValid())
	{
		OldSelection = GridsComboBox->GetSelectedItem();
	}

	Grids.Empty();
	GridOptions.Empty();

	if (CurrentDebuggingManager.IsValid() && CurrentDebuggingManager->GetGridNames(Grids))
	{
		GridOptions.Reserve(Grids.Num());
		for (const FName& GridNameItr : Grids)
		{
			GridOptions.Add(MakeShareable(new FString(GridNameItr.ToString())));
		}
	}

	// Restore Selection If Possible
	if (GridsComboBox.IsValid() && OldSelection.IsValid())
	{
		bool bMatchFound = false;
		for (int32 GridIdx = 0; GridIdx < GridOptions.Num(); GridIdx++)
		{
			if (*GridOptions[GridIdx] == *OldSelection)
			{
				GridsComboBox->SetSelectedItem(GridOptions[GridIdx]);
				bMatchFound = true;
				break;
			}
		}

		if (!bMatchFound)
		{
			GridsComboBox->ClearSelection();
		}
	}

	// Ensure we have a valid selection
	if (GridsComboBox.IsValid())
	{
		TSharedPtr<FString> CurrentSelection = GridsComboBox->GetSelectedItem();
		if (GridOptions.Find(CurrentSelection) == INDEX_NONE)
		{
			if (GridOptions.Num() > 0)
			{
				GridsComboBox->SetSelectedItem(GridOptions[0]);
			}
			else
			{
				GridsComboBox->ClearSelection();
			}
		}
	}
}

///////////////////////////
///// Initial Options /////
///////////////////////////

TSharedPtr<FString> SST_SparseGridHeatmapTab::GetInitialManagerOption()
{
	if (ensure(Managers.Num() == ManagerOptions.Num()))
	{
		if (CurrentDebuggingManager.IsValid())
		{
			for (int32 ManagerIndex = 0; ManagerIndex < Managers.Num(); ManagerIndex++)
			{
				if (Managers[ManagerIndex].IsValid() && (Managers[ManagerIndex].Get() == CurrentDebuggingManager.Get()))
				{
					return ManagerOptions[ManagerIndex];
				}
			}
		}
	}

	return ManagerOptions[0];
}

TSharedPtr<FString> SST_SparseGridHeatmapTab::GetInitialGridOption()
{
	if (ensure(Grids.Num() == GridOptions.Num()))
	{
		for (int32 NameIdx = 0; NameIdx < Grids.Num(); NameIdx++)
		{
			if (Grids[NameIdx] == CurrentDebuggingGrid)
			{
				return GridOptions[NameIdx];
			}
		}
	}

	return NULL;
}

/////////////////////////////
///// Options Selection /////
/////////////////////////////

void SST_SparseGridHeatmapTab::OnManagerSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	const TSharedPtr<FString> ManagerSelection = ManagersComboBox->GetSelectedItem();
	if (ensure(ManagerSelection.IsValid() && ManagerOptions.Num() == Managers.Num()))
	{
		int32 ManagerIndex = INDEX_NONE;
		for (int32 MIdx = 0; MIdx < ManagerOptions.Num(); MIdx++)
		{
			if (*ManagerOptions[MIdx] == *ManagerSelection)
			{
				ManagerIndex = MIdx;
				break;
			}
		}

		if (ensure(Managers.IsValidIndex(ManagerIndex)))
		{
			CurrentDebuggingManager = Managers[ManagerIndex];
		}		
	}
	else
	{
		CurrentDebuggingManager = nullptr;
	}

	GenerateGridOptions();
}

void SST_SparseGridHeatmapTab::OnGridSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	const TSharedPtr<FString> GridSelection = GridsComboBox->GetSelectedItem();

	CurrentDebuggingGrid = NAME_None;
	if (GridSelection.IsValid() && ensure(GridOptions.Num() == Grids.Num()))
	{
		for (int32 GIdx = 0; GIdx < GridOptions.Num(); GIdx++)
		{
			if (*GridOptions[GIdx] == *GridSelection)
			{
				CurrentDebuggingGrid = Grids[GIdx];
				break;
			}
		}
	}
	
	UpdateDebugTarget();
}

bool SST_SparseGridHeatmapTab::IsManagerComboBoxEnabled() const
{
	return Managers.Num() != 0;
}

bool SST_SparseGridHeatmapTab::IsGridComboBoxEnabled() const
{
	return CurrentDebuggingManager.IsValid() && Grids.Num();
}

void SST_SparseGridHeatmapTab::UpdateDebugTarget()
{
	if (HeatmapWidget.IsValid())
	{
		if (CurrentDebuggingGrid != NAME_None)
		{
			HeatmapWidget->SetGridManager(CurrentDebuggingManager.IsValid() ? CurrentDebuggingManager.Get() : nullptr, CurrentDebuggingGrid);
		}
		else
		{
			HeatmapWidget->SetGridManager(nullptr, NAME_None);
		}
	}
}

///////////////////////
///// Diagnostics /////
///////////////////////

void SST_SparseGridHeatmapTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Update Diagnostics Data
	if (CurrentDebuggingManager.IsValid() && CurrentDebuggingManager->GetGridConfig() && CurrentDebuggingGrid != NAME_None)
	{
		const int32 NumCells = CurrentDebuggingManager->GetGridConfig()->GetNumCellsX() * CurrentDebuggingManager->GetGridConfig()->GetNumCellsY();
		CurrentDebuggingManager->GetGridMemoryInfo(CurrentDebuggingGrid, TotalObjects, CellAllocSize, CellUsedSize, TotalAllocSize, TotalUsedSize);
	}
}

void SST_SparseGridHeatmapTab::ToggleDiagnostics()
{
	bShowingDiagnostics = !bShowingDiagnostics;
	SetCanTick(bShowingDiagnostics);
}

bool SST_SparseGridHeatmapTab::AreDiagnosticsVisible() const
{
	return bShowingDiagnostics && CurrentDebuggingGrid.IsValid();
}

EVisibility SST_SparseGridHeatmapTab::GetDiagnosticsVisibility() const
{
	return bShowingDiagnostics && CurrentDebuggingManager.IsValid() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

//////////////////////////////////
///// Diagnostics - Bindings /////
//////////////////////////////////

FText SST_SparseGridHeatmapTab::Diagnostics_GetCellMemoryText() const
{
	return FText::Format(LOCTEXT("CellMemoryValue", "{0} Bytes (Used) / {1} Bytes (Alloc)"), FText::AsNumber(CellUsedSize), FText::AsNumber(CellAllocSize));
}

TOptional<float> SST_SparseGridHeatmapTab::Diagnostics_GetCellMemoryRatio() const
{
	if (CellAllocSize == 0)
	{
		return 0.f;
	}
	else
	{
		return (float)((double)CellUsedSize / (double)CellAllocSize);
	}
}

FText SST_SparseGridHeatmapTab::Diagnostics_GetTotalObjectsText() const
{
	return FText::AsNumber(TotalObjects);
}

FText SST_SparseGridHeatmapTab::Diagnostics_GetTotalMemoryText() const
{
	return FText::Format(LOCTEXT("TotalMemoryValue", "{0} Bytes (Used) / {1} Bytes (Alloc)"), FText::AsNumber(TotalUsedSize), FText::AsNumber(TotalAllocSize));
}

TOptional<float> SST_SparseGridHeatmapTab::Diagnostics_GetTotalMemoryRatio() const
{
	if (TotalAllocSize == 0)
	{
		return 0.f;
	}
	else
	{
		return (float)((double)TotalUsedSize / (double)TotalAllocSize);
	}
}

/////////////////////
///// Delegates /////
/////////////////////

void SST_SparseGridHeatmapTab::OnPIEStarted(bool bIsSimulating)
{
	GenerateManagerOptions(false);
}

void SST_SparseGridHeatmapTab::OnPIEEnded(bool bIsSimulating)
{
	GenerateManagerOptions(false);
}

void SST_SparseGridHeatmapTab::OnMapChanged(uint32 InMapChangeFlags)
{
	if (InMapChangeFlags == MapChangeEventFlags::NewMap || InMapChangeFlags == MapChangeEventFlags::WorldTornDown)
	{
		GenerateManagerOptions(false);
	}
}

void SST_SparseGridHeatmapTab::OnNewCurrentLevel()
{
	GenerateManagerOptions(true);
}

//////////////////
///// Export /////
//////////////////

void SST_SparseGridHeatmapTab::ExportHeatMap()
{
	if (HeatmapWidget.IsValid() && CurrentDebuggingManager.IsValid())
	{
		const UTexture2D* Data = HeatmapWidget->GetDataBuffer();
		if (Data)
		{
			TArray<FColor> Pixels;
			const uint32 SizeX = (uint32)Data->GetSizeX();
			const uint32 SizeY = (uint32)Data->GetSizeY();
			Pixels.SetNumUninitialized(SizeX * SizeY);
			FColor* PixelData = Pixels.GetData();
 
 			// Read data on Render Thread
			ENQUEUE_RENDER_COMMAND(FST_SGExportHeatMapCommand)(
				[RHITexture = Data->Resource->TextureRHI, PixelData, SizeX, SizeY](FRHICommandListImmediate& RHICmdList)
 				{
 					uint32 DestStride = 0;
 					const FColor* SourceBuffer = static_cast<const FColor*>(RHILockTexture2D(RHITexture->GetTexture2D(), 0, RLM_ReadOnly, DestStride, false, false));

					const uint32 MaxStride = DestStride / sizeof(FColor);
					const uint32 CustomStride = SizeX * sizeof(FColor);
					
					// Texture will always be returned with padding to power of 2
					// Since grid can be any size, we have to fill row-by-row
					for (uint32 TexY = 0; TexY < SizeY; TexY++)
					{
						FMemory::Memcpy(PixelData + (TexY * SizeX), SourceBuffer + (TexY * MaxStride), CustomStride);
					}

 					RHIUnlockTexture2D(RHITexture->GetTexture2D(), 0, false, false);
 				});
 
 			// Important that data is up-to-date, pause GT until RT finishes.
 			FlushRenderingCommands();

			// Convert & Compress to PNG
			TArray<uint8> CompressedPNG;
			FImageUtils::CompressImageArray((int32)SizeX, (int32)SizeY, Pixels, CompressedPNG);

			// Save to Disk
			// Folder == Level Asset Name
			const UWorld* GridWorld = CurrentDebuggingManager->GetWorld();
			checkf(GridWorld != nullptr, TEXT("Invalid Grid World"));

			FString MapName = GridWorld->GetMapName();
			MapName.RemoveFromStart(GridWorld->StreamingLevelsPrefix);

			const FString HeatmapDir = FPaths::ProjectSavedDir() / TEXT("Heatmaps") / MapName;
			const FString FileName = FDateTime::Now().ToString();
			const FString OutputFull = FString::Printf(TEXT("%s/%s.png"), *HeatmapDir, *FileName);

			// If this fails, the write failed for IO reasons
			const bool bWroteFile = FFileHelper::SaveArrayToFile(CompressedPNG, *OutputFull);
			if (!bWroteFile)
			{
				UE_LOG(LogST_SparseGridEditor, Warning, TEXT("ExportHeatMap:: Writing Heatmap '%s' to Disk Failed."), *FileName);
			}
		}
	}
}

////////////////////////
///// Proxy Object /////
////////////////////////

void SST_SparseGridHeatmapTab::RefreshProxyDetailsPanel()
{

}

void SST_SparseGridHeatmapTab::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditorProxy);
}

#undef LOCTEXT_NAMESPACE