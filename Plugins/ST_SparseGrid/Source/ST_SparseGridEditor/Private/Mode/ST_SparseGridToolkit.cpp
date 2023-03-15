// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridToolkit.h"
#include "EditorModeManager.h"
#include "ST_SparseGridToolkitWidget.h"
#include "ST_SparseGridEditorModule.h"

#include "ST_SparseGridEdMode.h"
#include "ST_SparseGridData.h"
#include "ST_SparseGridCommands.h"

// Engine
#include "Widgets/Input/SNumericEntryBox.h"
#include "Misc/ScopedSlowTask.h"
#include "EngineUtils.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "NSSparseGridToolkit"

/////////////////////
///// Lifecycle /////
/////////////////////

FST_SparseGridToolkit::FST_SparseGridToolkit()
	: FModeToolkit()
{}

//////////////////////////
///// Initialization /////
//////////////////////////

void FST_SparseGridToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	// Setup Actions
	const FST_SparseGridEdMode* SparseGridMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	TSharedRef<FUICommandList> CommandList = SparseGridMode->GetUICommandList();

	CommandList->MapAction(FST_SparseGridCommands::Get().Cmd_FitToWorld, FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnFitToWorld), FCanExecuteAction::CreateSP(this, &FST_SparseGridToolkit::CanEditGridProperties)));
	CommandList->MapAction(FST_SparseGridCommands::Get().Cmd_VisitWebsite, FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnVisitWebsite)));
	CommandList->MapAction(FST_SparseGridCommands::Get().Cmd_Documentation, FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnViewDocumentation)));
	CommandList->MapAction(FST_SparseGridCommands::Get().Cmd_SupportThread, FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnViewSupportThread)));
	CommandList->MapAction(FST_SparseGridCommands::Get().Cmd_ShowHeatmap, FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnShowHeatmap)));

	// Create Widget
	SAssignNew(ModeToolsWidget, SST_SparseGridToolkitWidget, SharedThis(this));

	FModeToolkit::Init(InitToolkitHost);

	RefreshGridCreationState();
}

//////////////////////////
///// Header Toolbar /////
//////////////////////////

void FST_SparseGridToolkit::OnVisitWebsite()
{
	static const FString STURL = TEXT("http://www.jambax.co.uk");
	if (!STURL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*STURL, nullptr, nullptr);
	}
}

void FST_SparseGridToolkit::OnViewDocumentation()
{
	static const FString DOCSURL = TEXT("https://drive.google.com/open?id=1RcE0R2iyXhSY2f4-Z_Idok3JhGyOf8pm");
	if (!DOCSURL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*DOCSURL, nullptr, nullptr);
	}
}

void FST_SparseGridToolkit::OnViewSupportThread()
{
	static const FString FORUMURL = TEXT("https://forums.unrealengine.com/unreal-engine/marketplace/1428507-sparse-grid-plugin-by-stormtide-ltd");
	if (!FORUMURL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*FORUMURL, nullptr, nullptr);
	}
}

///////////////////
///// Toolkit /////
///////////////////

bool FST_SparseGridToolkit::CanEditGridProperties() const
{
	// Don't allow edits in PIE
	const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	const UST_SparseGridData* GridData = EdMode->GetContextGridData();

	if (EdMode != nullptr && GridData != nullptr && GEditor && GEditor->PlayWorld == nullptr)
	{
		return true;
	}

	return false;
}

void FST_SparseGridToolkit::OnFitToWorld()
{
	FScopedSlowTask SlowTask(2.f, LOCTEXT("FitToWorldTask", "Fit Grid To World"));
	SlowTask.MakeDialog(false, false);

	const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	UST_SparseGridData* GridData = EdMode ? EdMode->GetContextGridData() : nullptr;
	if (GridData)
	{
		SlowTask.EnterProgressFrame(1.f, LOCTEXT("FitToWorld_Cleanup", "Cleaning Up Actors"));
		GEngine->PerformGarbageCollectionAndCleanupActors();

		SlowTask.EnterProgressFrame(1.f, LOCTEXT("FitToWorld_MinMax", "Computing Grid Min/Max"));
		FVector WorldMin = FVector(WORLD_MAX);
		FVector WorldMax = FVector(-WORLD_MAX);

		for (TActorIterator<AActor> ActorItr(EdMode->GetWorld()); ActorItr; ++ActorItr)
		{
			const AActor* WorldActor = *ActorItr;
			const FVector Location = WorldActor->GetActorLocation();

			// TODO: Add Special case for Landscape actors, since we can *probably* traverse the entire landscape
			WorldMin.X = FMath::Min(WorldMin.X, Location.X);
			WorldMin.Y = FMath::Min(WorldMin.Y, Location.Y);
			WorldMin.Z = FMath::Min(WorldMin.Z, Location.Z);

			WorldMax.X = FMath::Max(WorldMax.X, Location.X);
			WorldMax.Y = FMath::Max(WorldMax.Y, Location.Y);
			WorldMax.Z = FMath::Max(WorldMax.Z, Location.Z);
		}

		GridData->FitToWorldBounds(WorldMin, WorldMax);
	}
}

void FST_SparseGridToolkit::OnModifyDensity(const int32 InDensityModifier, const ETextCommit::Type CommitType/* = ETextCommit::Default*/) const
{
	// Grid determines how it rescales itself
	const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	UST_SparseGridData* GridData = EdMode ? EdMode->GetContextGridData() : nullptr;
	if (GridData)
	{
		GridData->ModifyDensity(InDensityModifier);
	}

	// Refresh Viewports
	GEditor->RedrawLevelEditingViewports(false);
}

void FST_SparseGridToolkit::OnShowHeatmap()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FST_SparseGridEditorModule::HeatmapTabName);
}

///////////////////////////////////
///// Grid Creation & Removal /////
///////////////////////////////////

void FST_SparseGridToolkit::OnCreateGrid(UClass* InGridDataClass)
{
	if (InGridDataClass)
	{
		// Try to create a new Grid asset for this world
		const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
		const UWorld* ReferenceWorld = EdMode ? EdMode->GetWorld() : nullptr;

		if (ReferenceWorld)
		{
			ULevel* PersistentLvl = ReferenceWorld->PersistentLevel;
			UST_SparseGridData* GridData = PersistentLvl->GetAssetUserData<UST_SparseGridData>();

			if (GridData)
			{
				PersistentLvl->RemoveUserDataOfClass(UST_SparseGridData::StaticClass());
				GridData->ConditionalBeginDestroy();
				GridData = nullptr;
			}

			GridData = NewObject<UST_SparseGridData>(ReferenceWorld->PersistentLevel, InGridDataClass, NAME_None, RF_Transactional);
			checkf(GridData != nullptr, TEXT("Invalid Data"));
			GridData->RegisterEditorWorld(ReferenceWorld);
			PersistentLvl->AddAssetUserData(GridData);
			PersistentLvl->MarkPackageDirty();
			

			// Refresh Toolkit
			ModeToolsWidget->RefreshGridDetailsPanel();
		}

		RefreshGridCreationState();
	}
}

void FST_SparseGridToolkit::OnRemoveGrid()
{
	if (bHasAnyGridData)
	{
		// Try to create a new Grid asset for this world
		const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
		const UWorld* ReferenceWorld = EdMode ? EdMode->GetWorld() : nullptr;

		if (ReferenceWorld)
		{
			ULevel* PersistentLvl = ReferenceWorld->PersistentLevel;
			UST_SparseGridData* GridData = PersistentLvl->GetAssetUserData<UST_SparseGridData>();

			if (GridData)
			{
				PersistentLvl->RemoveUserDataOfClass(UST_SparseGridData::StaticClass());
				PersistentLvl->MarkPackageDirty();

				GridData->ConditionalBeginDestroy();
				GridData = nullptr;

				// Refresh Toolkit
				ModeToolsWidget->RefreshGridDetailsPanel();
			}
		}

		RefreshGridCreationState();
	}
}

void FST_SparseGridToolkit::RefreshGridCreationState()
{
	// Try to create a new Grid asset for this world
	const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	const UWorld* ReferenceWorld = EdMode ? EdMode->GetWorld() : nullptr;

	if (ReferenceWorld)
	{
		ULevel* PersistentLvl = ReferenceWorld->PersistentLevel;
		if (PersistentLvl)
		{
			bHasAnyGridData = PersistentLvl->GetAssetUserData<UST_SparseGridData>() ? true : false;
			return;
		}
	}

	bHasAnyGridData = false;
}

//////////////////////////
///// Grid Combo Box /////
//////////////////////////

TSharedRef<SWidget> FST_SparseGridToolkit::GenerateGridMenuContent()
{
	const FST_SparseGridEdMode* EdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	check(EdMode);
	
	const UWorld* ReferenceWorld = EdMode->GetWorld();
	check(ReferenceWorld);

	// Copy GridDataClasses
	// Remove the class that's already in use if one exists
	TArray<UClass*> GridDataClasses = EdMode->GetGridDataClasses();	
	UClass* ExcludedClass = nullptr;
	if (bHasAnyGridData)
	{
		ULevel* PersistentLvl = ReferenceWorld->PersistentLevel;
		if (ensure(PersistentLvl))
		{
			UST_SparseGridData* ExistingData = PersistentLvl->GetAssetUserData<UST_SparseGridData>();
			if (ensure(ExistingData))
			{
				ExcludedClass = ExistingData->GetClass();
				GridDataClasses.Remove(ExcludedClass);
			}
		}
	}

	// Create the Menu. 'Add' Categories come first
	FMenuBuilder NewMenu = FMenuBuilder(true, EdMode->GetUICommandList());
	{
		// Add the 'Add' or 'Replace With' Classes
		if (GridDataClasses.Num() > 0)
		{
			NewMenu.BeginSection("SparseGridClasses", !bHasAnyGridData ? LOCTEXT("AddMenu", "Create New") : LOCTEXT("ModifyMenu", "Replace With"));
			{
				// Sort Classes by display name Alphabetically
				GridDataClasses.Sort([&](const UClass& A, const UClass& B)
					{ return (A.GetDisplayNameText().CompareTo(B.GetDisplayNameText(), ETextComparisonLevel::Default) == -1); }
				);

				// Add Each Class
				for (UClass* ClassItr : GridDataClasses)
				{
					NewMenu.AddMenuEntry(
						ClassItr->GetDisplayNameText(),
						ClassItr->GetToolTipText(false),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnCreateGrid, ClassItr))
					);
				}
			}
			NewMenu.EndSection();
		}

		// Add the 'Remove' Class
		if (bHasAnyGridData && ExcludedClass)
		{
			NewMenu.BeginSection("SparseGridRemove", LOCTEXT("RemoveMenu", "Remove"));
			{
				NewMenu.AddMenuEntry(
					ExcludedClass->GetDisplayNameText(),
					LOCTEXT("RemoveTooltip", "Removes the grid data from the world"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnRemoveGrid))
				);
			}
			NewMenu.EndSection();
		}
	}

	return NewMenu.MakeWidget();
}

/////////////////////////////
///// Density Combo Box /////
/////////////////////////////

TSharedRef<SWidget> FST_SparseGridToolkit::GenerateDensityMenuContent() const
{
	FMenuBuilder NewMenu = FMenuBuilder(true, NULL);

	// Add Increase Density Quick-Functions
	NewMenu.BeginSection("IncreaseDensity", LOCTEXT("IncreaseDensity", "Add Cells"));
	{
		AddDensityEntry(NewMenu, 5);
		AddDensityEntry(NewMenu, 10);
		AddDensityEntry(NewMenu, 15);
		AddDensityEntry(NewMenu, 20);
		AddDensityEntry(NewMenu, 25);
	}
	NewMenu.EndSection();

	// Add Decrease Density Quick-Functions
	NewMenu.BeginSection("DecreaseDensity", LOCTEXT("DecreaseDensity", "Remove Cells"));
	{
		AddDensityEntry(NewMenu, -5);
		AddDensityEntry(NewMenu, -10);
		AddDensityEntry(NewMenu, -15);
		AddDensityEntry(NewMenu, -20);
		AddDensityEntry(NewMenu, -25);
	}
	NewMenu.EndSection();
	
	// Add 'Custom' Density Entry Box
	NewMenu.BeginSection("CustomDensity", LOCTEXT("CustomDensity", "Custom"));
	{
		NewMenu.AddWidget(CreateDensityCustomEntry(), LOCTEXT("CustomDensity", "Delta"));
	}
	NewMenu.EndSection();

	return NewMenu.MakeWidget();
}

void FST_SparseGridToolkit::AddDensityEntry(FMenuBuilder& InBuilder, const int32 InModifier) const
{
	const int32 Pos = FMath::Clamp(InModifier, 0, 1);

	FFormatOrderedArguments FormatArgs;
	FormatArgs.Add(Pos ? LOCTEXT("Add", "Add") : LOCTEXT("Remove", "Remove"));
	FormatArgs.Add(FText::AsNumber(FMath::Abs(InModifier)));
	FormatArgs.Add(Pos ? LOCTEXT("Increasing", "increasing") : LOCTEXT("Decreasing", "decreasing"));

	InBuilder.AddMenuEntry(
		FText::Format(LOCTEXT("DensityMenuEntry", "{0}"), FText::AsNumber(InModifier)),
		FText::Format(LOCTEXT("DensityMenuTooltip", "{0} {1} cells, {2} grid density."), FormatArgs),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FST_SparseGridToolkit::OnModifyDensity, InModifier, ETextCommit::Default))
	);
}

TSharedRef<SWidget> FST_SparseGridToolkit::CreateDensityCustomEntry() const
{
	return SNew(SBox)
	.HAlign(HAlign_Right)
	[
		SNew(SBox)
		.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
		.WidthOverride(40.f)
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(false)
			.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
			.Value(0)
			.OnValueCommitted(this, &FST_SparseGridToolkit::OnModifyDensity)
		]
	];
}

/////////////////////
///// Accessors /////
/////////////////////

FEdMode* FST_SparseGridToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FName("SparseGridEditorMode"));
}

TSharedPtr<SWidget> FST_SparseGridToolkit::GetInlineContent() const
{
	return ModeToolsWidget;
}

#undef LOCTEXT_NAMESPACE