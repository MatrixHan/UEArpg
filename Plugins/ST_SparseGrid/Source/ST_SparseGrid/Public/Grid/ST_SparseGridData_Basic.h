// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "ST_SparseGridData.h"
#include "ST_SparseGridData_Basic.generated.h"

/*
* Support for the ST_SparseGridComponent
*/
UCLASS(meta = (DisplayName = "Sparse Grid Data - Basic"))
class ST_SPARSEGRID_API UST_SparseGridData_Basic : public UST_SparseGridData
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridData_Basic(const FObjectInitializer& OI);
};