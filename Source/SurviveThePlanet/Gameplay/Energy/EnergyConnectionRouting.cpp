#include "Gameplay/Energy/EnergyConnectionRouting.h"

bool FEnergyConnectionRouting::IsSegmentClear(const FVector2D& Start, const FVector2D& End, const TArray<FBox2D>& Obstacles)
{
 const FVector2D Direction = End - Start;
 for (const FBox2D& Box : Obstacles)
 {
  double Entry = 0.0, Exit = 1.0;
  bool bIntersects = true;
  for (int32 Axis = 0; Axis < 2; ++Axis)
  {
   if (FMath::Abs(Direction[Axis]) < UE_DOUBLE_SMALL_NUMBER)
   {
    if (Start[Axis] < Box.Min[Axis] || Start[Axis] > Box.Max[Axis]) bIntersects = false;
    continue;
   }
   double Near = (Box.Min[Axis] - Start[Axis]) / Direction[Axis];
   double Far = (Box.Max[Axis] - Start[Axis]) / Direction[Axis];
   if (Near > Far) Swap(Near, Far);
   Entry = FMath::Max(Entry, Near);
   Exit = FMath::Min(Exit, Far);
   if (Entry > Exit) bIntersects = false;
  }
  if (bIntersects) return false;
 }
 return true;
}

bool FEnergyConnectionRouting::FindPath(const FVector2D& Start, const FVector2D& End, const TArray<FBox2D>& Obstacles, TArray<FVector2D>& OutPath)
{
 OutPath.Reset();
 if (IsSegmentClear(Start, End, Obstacles))
 {
  OutPath = {Start, End};
  return true;
 }
 TArray<FVector2D> Nodes = {Start, End};
 FBox2D SearchBounds(ForceInit);
 SearchBounds += Start;
 SearchBounds += End;
 SearchBounds = SearchBounds.ExpandBy(4000.0);
 TArray<FBox2D> CandidateBoxes;
 for (const FBox2D& Box : Obstacles)
  if (SearchBounds.Intersect(Box)) CandidateBoxes.Add(Box);
 auto DistanceToRoute = [&](const FBox2D& Box)
 {
  const FVector2D Direction = End-Start;
  const double T = FMath::Clamp(FVector2D::DotProduct(Box.GetCenter()-Start, Direction) / FMath::Max(Direction.SizeSquared(),1.0),0.0,1.0);
  return Box.ComputeSquaredDistanceToPoint(Start + T*Direction);
 };
 CandidateBoxes.StableSort([&](const FBox2D& A, const FBox2D& B) { return DistanceToRoute(A) < DistanceToRoute(B); });
 for (const FBox2D& Box : CandidateBoxes)
 {
  // Bound preview work; if no route is found, construction still remains allowed.
  if (Nodes.Num() + 4 > 258) break;
  const FBox2D Corners = Box.ExpandBy(2.0);
  for (const FVector2D& Corner : {Corners.Min, FVector2D(Corners.Min.X, Corners.Max.Y),
       Corners.Max, FVector2D(Corners.Max.X, Corners.Min.Y)})
  {
   bool bInsideOther = false;
   for (const FBox2D& Other : Obstacles)
    bInsideOther |= Other.IsInside(Corner);
   if (!bInsideOther) Nodes.Add(Corner);
  }
 }
 TArray<double> Costs;
 TArray<int32> Previous;
 TArray<bool> Closed;
 Costs.Init(TNumericLimits<double>::Max(), Nodes.Num());
 Previous.Init(INDEX_NONE, Nodes.Num());
 Closed.Init(false, Nodes.Num());
 Costs[0] = 0.0;
 for (int32 Iteration = 0; Iteration < Nodes.Num(); ++Iteration)
 {
  int32 Current = INDEX_NONE;
  double Best = TNumericLimits<double>::Max();
  for (int32 Index = 0; Index < Nodes.Num(); ++Index)
  {
   const double Score = Costs[Index] + FVector2D::Distance(Nodes[Index], End);
   if (!Closed[Index] && Score < Best) { Best = Score; Current = Index; }
  }
  if (Current == INDEX_NONE) return false;
  if (Current == 1)
  {
   for (int32 Index = 1; Index != INDEX_NONE; Index = Previous[Index]) OutPath.Insert(Nodes[Index], 0);
   return true;
  }
  Closed[Current] = true;
  for (int32 Next = 0; Next < Nodes.Num(); ++Next)
  {
   if (Closed[Next]) continue;
   const double Cost = Costs[Current] + FVector2D::Distance(Nodes[Current], Nodes[Next]);
   if (Cost >= Costs[Next] || !IsSegmentClear(Nodes[Current], Nodes[Next], Obstacles)) continue;
   Costs[Next] = Cost;
   Previous[Next] = Current;
  }
 }
 return false;
}
