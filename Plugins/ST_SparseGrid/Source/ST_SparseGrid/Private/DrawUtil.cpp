// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawUtil.h"

DrawUtil::DrawUtil()
{
}

DrawUtil::~DrawUtil()
{
}


ULineBatchComponent* DrawUtil::GetDebugLineBatcher(const UWorld* InWorld, bool bPersistentLines, float LifeTime, bool bDepthIsForeground)
{
	return (InWorld ? (bDepthIsForeground ? InWorld->ForegroundLineBatcher : ((bPersistentLines || (LifeTime > 0.f)) ? InWorld->PersistentLineBatcher : InWorld->LineBatcher)) : nullptr);
}


void DrawUtil::DrawDebugMesh(const UWorld* InWorld, TArray<FVector> const& Verts, TArray<int32> const& Indices, FColor const& Color, bool bPersistent, float LifeTime, uint8 DepthPriority)
{
	if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer)
	{
		if (ULineBatchComponent* const LineBatcher = GetDebugLineBatcher(InWorld, bPersistent, LifeTime, false))
		{
			float const ActualLifetime = GetDebugLineLifeTime(LineBatcher, LifeTime, bPersistent);
			LineBatcher->DrawMesh(Verts, Indices, Color, DepthPriority, ActualLifetime);
		}
	}
}
void DrawUtil::DrawDebugLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines , float LifeTime , uint8 DepthPriority , float Thickness )
{
	if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer)
	{
		// this means foreground lines can't be persistent 
		if (ULineBatchComponent* const LineBatcher = GetDebugLineBatcher(InWorld, bPersistentLines, LifeTime, (DepthPriority == SDPG_Foreground)))
		{
			float const LineLifeTime = GetDebugLineLifeTime(LineBatcher, LifeTime, bPersistentLines);
			LineBatcher->DrawLine(LineStart, LineEnd, Color, DepthPriority, Thickness, LineLifeTime);
		}
	}
}

void DrawUtil::DrawDebugSolidPlane(const UWorld* InWorld, FPlane const& P, FVector const& Loc, float Size, FColor const& Color, bool bPersistent , float LifeTime , uint8 DepthPriority )
{
	DrawDebugSolidPlane(InWorld, P, Loc, FVector2D(Size, Size), Color, bPersistent, LifeTime, DepthPriority);
}
void DrawUtil::DrawDebugSolidPlane(const UWorld* InWorld, FPlane const& P, FVector const& Loc, FVector2D const& Extents, FColor const& Color, bool bPersistent , float LifeTime, uint8 DepthPriority)
{
	// no debug line drawing on dedicated server
	if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer)
	{
		FVector const ClosestPtOnPlane = Loc - P.PlaneDot(Loc) * P;

		FVector U, V;
		P.FindBestAxisVectors(U, V);
		U *= Extents.Y;
		V *= Extents.X;

		TArray<FVector> Verts;
		Verts.AddUninitialized(4);
		Verts[0] = ClosestPtOnPlane + U + V;
		Verts[1] = ClosestPtOnPlane - U + V;
		Verts[2] = ClosestPtOnPlane + U - V;
		Verts[3] = ClosestPtOnPlane - U - V;

		TArray<int32> Indices;
		Indices.AddUninitialized(6);
		Indices[0] = 0; Indices[1] = 2; Indices[2] = 1;
		Indices[3] = 1; Indices[4] = 2; Indices[5] = 3;

		// plane quad
		DrawDebugMesh(InWorld, Verts, Indices, Color, bPersistent, LifeTime, DepthPriority);

		// arrow indicating normal
		DrawDebugDirectionalArrow(InWorld, ClosestPtOnPlane, ClosestPtOnPlane + P * 16.f, 8.f, FColor::White, bPersistent, LifeTime, DepthPriority);
	}
}


void DrawUtil::DrawDebugDirectionalArrow(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, float ArrowSize, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	// no debug line drawing on dedicated server
	if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer)
	{
		if (ArrowSize <= 0)
		{
			ArrowSize = 10.f;
		}

		DrawDebugLine(InWorld, LineStart, LineEnd, Color, bPersistentLines, LifeTime, DepthPriority, Thickness);

		FVector Dir = (LineEnd - LineStart);
		Dir.Normalize();
		FVector Up(0, 0, 1);
		FVector Right = Dir ^ Up;
		if (!Right.IsNormalized())
		{
			Dir.FindBestAxisVectors(Up, Right);
		}
		FVector Origin = FVector::ZeroVector;
		FMatrix TM;
		// get matrix with dir/right/up
		TM.SetAxes(&Dir, &Right, &Up, &Origin);

		// since dir is x direction, my arrow will be pointing +y, -x and -y, -x
		float ArrowSqrt = FMath::Sqrt(ArrowSize);
		FVector ArrowPos;
		DrawDebugLine(InWorld, LineEnd, LineEnd + TM.TransformPosition(FVector(-ArrowSqrt, ArrowSqrt, 0)), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
		DrawDebugLine(InWorld, LineEnd, LineEnd + TM.TransformPosition(FVector(-ArrowSqrt, -ArrowSqrt, 0)), Color, bPersistentLines, LifeTime, DepthPriority, Thickness);
	}
}

void DrawUtil::DrawFlush(const UWorld* InWorld, bool bPersistentLines, float LifeTime, bool bDepthIsForeground)
{
	GetDebugLineBatcher(InWorld, bPersistentLines, LifeTime, bDepthIsForeground)->Flush();
}