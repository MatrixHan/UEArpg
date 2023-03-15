// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ST_SparseGridTypes.h"
#include "ST_SparseGridStatics.generated.h"

/*
* Sparse-Grid Statics
*
* Function Library for common maths functions in the Sparse Grid system
* Some functionality exposed to Blueprint so that users can build their own search queries.
*/
UCLASS(meta = (DisplayName = "Sparse Grid Function Library"))
class ST_SPARSEGRID_API UST_SparseGridStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	////////////////////////////////////////
	///// Useful Array Functions - C++ /////
	////////////////////////////////////////

	/* Get the Index in a 1D array when treating it as 2D */
	FORCEINLINE static int32 Get2DArray_Index(const int32 Column, const int32 Row, const int32 NumRows)
	{
		return Row + Column * NumRows;
	}

	/* Get the Column and Row from the Index of a 1D Array treated as 2D.*/
	FORCEINLINE static void Get2DArray_ColumnAndRow(int32& OutColumn, int32& OutRow, const int32 NumRows, const int32 InIndex)
	{
#if !(UE_BUILD_SHIPPING)
		checkfSlow(NumRows > 0, TEXT("Get2DArray_ColumnAndRow() - Divide By Zero!"));
		checkfSlow(InIndex >= 0, TEXT("Get2DArray_ColumnAndRow() - Invalid Index!"));
#endif
		OutColumn = InIndex % NumRows;
		OutRow = InIndex / NumRows;
	}
	
	/*
	* Convert FST_GridRef2D to FVector2D
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Conversions", meta = (CompactNodeTitle = "->", DisplayName = "To Vector2D (GridRef2D)", BlueprintAutocast))
	static FVector2D GridRef2DToVector(const FST_GridRef2D& InGridRef2D) { return InGridRef2D.ToVector(); }
};