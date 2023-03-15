// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridToolkitWidget.h"

// Editor
#include "DetailLayoutBuilder.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Editor/EditorStyle/Private/SlateEditorStyle.h"

// Extras
#include "ST_SparseGridEdMode.h"
#include "ST_SparseGridData.h"
#include "ST_SparseGridStyle.h"
#include "ST_SparseGridCommands.h"
#include "ST_SparseGridToolkit.h"

// Slate
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "NSSparseGridToolkitWidget"

///////////////////////
///// Constructor /////
///////////////////////

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SST_SparseGridToolkitWidget::Construct(const FArguments& InArgs, TSharedRef<FST_SparseGridToolkit> InParentToolkit)
{
	// Create Custom Toolbar
	TSharedRef<FUICommandList> CommandList = InParentToolkit->GetToolkitCommands();

	// Create Embedded Details View (Grid)
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs = FDetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);

	Grid_DetailsPanel = PropertyModule.CreateDetailView(DetailsViewArgs);
	Grid_DetailsPanel->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(InParentToolkit, &FST_SparseGridToolkit::CanEditGridProperties));

	// Refresh Details Panel
	RefreshGridDetailsPanel();

	// Create Widget Content
	ChildSlot.VAlign(VAlign_Top)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Top).HAlign(HAlign_Fill)
		[
			BuildHeader(CommandList, InParentToolkit)
		]
		+SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder")).HAlign(HAlign_Left)
			[
				BuildSparseGridToolbar(CommandList, InParentToolkit)
			]
		]
		+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Top).HAlign(HAlign_Fill)
		[
			BuildSparseGridInfoBar(InParentToolkit)
		]
		+SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Top).HAlign(HAlign_Fill)
		[
			Grid_DetailsPanel.ToSharedRef()
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

//////////////////////////////
///// Toolbar Generation /////
//////////////////////////////

TSharedRef<SWidget> SST_SparseGridToolkitWidget::BuildSparseGridToolbar(const TSharedRef<FUICommandList> InCommandList, TSharedRef<FST_SparseGridToolkit> InParentToolkit) const
{
	FToolBarBuilder ReturnToolbar = FToolBarBuilder(InCommandList, FMultiBoxCustomization::None);

	// Add Grid Data Combo Box
	ReturnToolbar.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(InParentToolkit, &FST_SparseGridToolkit::GenerateGridMenuContent),
		LOCTEXT("GridCombo_Label", "Grid Data"),
		LOCTEXT("GridCombo_Tooltip", "Add, Modify or Remove Grid Data"),
		FSlateIcon(FST_SparseGridStyle::GetStyleSetName(), "SparseGrid.TabIcon"),
		false
	);

	ReturnToolbar.AddSeparator();

	// Add Fit-To-World Button
	ReturnToolbar.AddToolBarButton(
		FST_SparseGridCommands::Get().Cmd_FitToWorld,
		NAME_None,
		LOCTEXT("Cmd.FitToWorld", "Fit To World"),
		LOCTEXT("Cmd.FitToWorld.Tooltip", "Fit the sparse grid to the world"),
		FSlateIcon(FSlateEditorStyle::GetStyleSetName(), "CurveEditor.Fit", "CurveEditor.Fit.Small")
	);

	// Add Density Combo Box
	ReturnToolbar.AddComboButton(
		FUIAction(FExecuteAction(), FCanExecuteAction::CreateSP(InParentToolkit, &FST_SparseGridToolkit::CanEditGridProperties)),
		FOnGetContent::CreateSP(InParentToolkit, &FST_SparseGridToolkit::GenerateDensityMenuContent),
		LOCTEXT("DensityCombo_Label", "Cell Density"),
		LOCTEXT("DensityCombo_Tooltip", "Modify grid density without affecting it's coverage.\r\nSome manual adjustment may be needed after multiple changes."),
		FSlateIcon(FSlateEditorStyle::GetStyleSetName(), "CurveEditor.Fit", "CurveEditor.Fit.Small")
	);

	// Add 'Show Heatmap' Tab Invoker
	ReturnToolbar.AddToolBarButton(
		FST_SparseGridCommands::Get().Cmd_ShowHeatmap,
		NAME_None,
		LOCTEXT("Cmd.ShowHeatmap", "Show Heatmap"),
		LOCTEXT("Cmd.ShowHeatmap.Tooltip", "Brings up the Heatmap tool to help diagnose grid efficiency"),
		FSlateIcon(FST_SparseGridStyle::GetStyleSetName(), "SparseGrid.HeatmapIcon", "SparseGrid.HeatmapIcon.Small")
	);

	return ReturnToolbar.MakeWidget();
}

TSharedRef<SWidget> SST_SparseGridToolkitWidget::BuildHeader(const TSharedRef<FUICommandList> InCommandList, TSharedRef<FST_SparseGridToolkit> InParentToolkit) const
{
	FToolBarBuilder HeaderToolbar = FToolBarBuilder(InCommandList, FMultiBoxCustomization::None, nullptr, true);
	HeaderToolbar.SetLabelVisibility(EVisibility::Collapsed);

	// Documentation Button
	HeaderToolbar.AddToolBarButton(
		FST_SparseGridCommands::Get().Cmd_Documentation,
		NAME_None,
		FText(),
		LOCTEXT("Cmd.Documentation.Tooltip", "View Documentation"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.VisitSearchForAnswersPage", "MainFrame.VisitSearchForAnswersPage")
	);

	// Add Support Button
	HeaderToolbar.AddToolBarButton(
		FST_SparseGridCommands::Get().Cmd_SupportThread,
		NAME_None,
		FText(),
		LOCTEXT("Cmd.Support.Tooltip", "Get Support"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.VisitForums", "MainFrame.VisitForums")
	);

	// Add Visit Website Button
	HeaderToolbar.AddToolBarButton(
		FST_SparseGridCommands::Get().Cmd_VisitWebsite,
		NAME_None,
		FText(),
		LOCTEXT("Cmd.Visit.Tooltip", "Visit Website"),
		FSlateIcon(FST_SparseGridStyle::GetStyleSetName(), "SparseGrid.StormtideIcon")
	);

	// Create Root Horizontal Box
	TSharedPtr<SBorder> RootBorder = SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(3.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("Title", "Sparse Grid System")).Font(FST_SparseGridStyle::GetPluginFont(14)).Justification(ETextJustify::Center)
			]
			+SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				HeaderToolbar.MakeWidget()
			]
		];

	return RootBorder.ToSharedRef();
}

TSharedRef<SWidget> SST_SparseGridToolkitWidget::BuildSparseGridInfoBar(TSharedRef<FST_SparseGridToolkit> InParentToolkit) const
{
	TSharedPtr<SBorder> InfoBorder = SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(3.f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(this, &SST_SparseGridToolkitWidget::GetGridNameText).Font(FST_SparseGridStyle::GetPluginFont(12)).Justification(ETextJustify::Center)
			]
			+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				SNew(STextBlock).Text(this, &SST_SparseGridToolkitWidget::GetDiagnosticText).Font(FST_SparseGridStyle::GetPluginFont(10))
			]
		];

	return InfoBorder.ToSharedRef();
}

///////////////////////////////
///// Manager Information /////
///////////////////////////////

FText SST_SparseGridToolkitWidget::GetGridNameText() const
{
	const FST_SparseGridEdMode* EditorMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	const UST_SparseGridData* GridData = EditorMode ? EditorMode->GetContextGridData() : nullptr;
	if (GridData)
	{
		return GridData->GetClass()->GetDisplayNameText();
	}

	return FText(LOCTEXT("NoGrid", "No Grid Data"));
}

FText SST_SparseGridToolkitWidget::GetDiagnosticText() const
{
	const FST_SparseGridEdMode* EditorMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	const UST_SparseGridData* GridData = EditorMode ? EditorMode->GetContextGridData() : nullptr;
	if (GridData)
	{
		return GridData->GetGridDiagnosticText();
	}

	return FText();
}

//////////////////////////////
///// Details Panel Host /////
//////////////////////////////

void SST_SparseGridToolkitWidget::RefreshGridDetailsPanel()
{
	const FST_SparseGridEdMode* SparseGridEdMode = FST_SparseGridEdMode::GetSparseGridEditorMode();
	if (SparseGridEdMode)
	{
		Grid_DetailsPanel->SetObject(SparseGridEdMode->GetContextGridData(), true);
	}
}

#undef LOCTEXT_NAMESPACE