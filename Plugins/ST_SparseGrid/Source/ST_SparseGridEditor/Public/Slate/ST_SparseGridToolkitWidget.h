// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

// Declarations
class IDetailsView;
class FST_SparseGridToolkit;
class FUICommandList;

class SST_SparseGridToolkitWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SST_SparseGridToolkitWidget) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FST_SparseGridToolkit> InParentToolkit);
	void RefreshGridDetailsPanel();

protected:
	// Tools
	FText GetGridNameText() const;
	FText GetDiagnosticText() const;

	TSharedRef<SWidget> BuildHeader(const TSharedRef<FUICommandList> InCommandList, TSharedRef<FST_SparseGridToolkit> InParentToolkit) const;
	TSharedRef<SWidget> BuildSparseGridToolbar(const TSharedRef<FUICommandList> InCommandList, TSharedRef<FST_SparseGridToolkit> InParentToolkit) const;
	TSharedRef<SWidget> BuildSparseGridInfoBar(TSharedRef<FST_SparseGridToolkit> InParentToolkit) const;

	// Details Panel Host
	TSharedPtr<IDetailsView> Grid_DetailsPanel;
};