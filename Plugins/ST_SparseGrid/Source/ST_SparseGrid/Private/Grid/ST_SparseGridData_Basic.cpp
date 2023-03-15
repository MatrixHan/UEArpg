// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridData_Basic.h"
#include "ST_SparseGridManager_Basic.h"

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridData_Basic::UST_SparseGridData_Basic(const FObjectInitializer& OI)
	: Super(OI)
{
	ManagerClass = UST_SparseGridManager_Basic::StaticClass();
}