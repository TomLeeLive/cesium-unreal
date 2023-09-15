// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumOriginShiftComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumWgs84Ellipsoid.h"
#include "LevelInstance/LevelInstanceActor.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

ECesiumOriginShiftMode UCesiumOriginShiftComponent::GetMode() const {
  return this->Mode;
}

void UCesiumOriginShiftComponent::SetMode(ECesiumOriginShiftMode NewMode) {
  this->Mode = NewMode;
}

double UCesiumOriginShiftComponent::GetDistance() const {
  return this->Distance;
}

void UCesiumOriginShiftComponent::SetDistance(double NewDistance) {
  this->Distance = NewDistance;
}

UCesiumOriginShiftComponent::UCesiumOriginShiftComponent() : Super() {
  this->PrimaryComponentTick.bCanEverTick = true;
  this->PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
  this->bAutoActivate = true;
}

void UCesiumOriginShiftComponent::OnRegister() {
  Super::OnRegister();
  this->ResolveGlobeAnchor();
}

void UCesiumOriginShiftComponent::BeginPlay() {
  Super::BeginPlay();
  this->ResolveGlobeAnchor();
}

namespace {
/**
 * @brief Clamping addition.
 *
 * Returns the sum of the given values, clamping the result to
 * the minimum/maximum value that can be represented as a 32 bit
 * signed integer.
 *
 * @param f The floating point value
 * @param i The integer value
 * @return The clamped result
 */
int32 clampedAdd(double f, int32 i) {
  int64 sum = static_cast<int64>(f) + static_cast<int64>(i);
  int64 min = static_cast<int64>(TNumericLimits<int32>::Min());
  int64 max = static_cast<int64>(TNumericLimits<int32>::Max());
  int64 clamped = FMath::Max(min, FMath::Min(max, sum));
  return static_cast<int32>(clamped);
}
} // namespace

void UCesiumOriginShiftComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!this->IsActive() || this->Mode == ECesiumOriginShiftMode::Disabled)
    return;

  if (!this->GlobeAnchor)
    return;

  ACesiumGeoreference* Georeference = this->GlobeAnchor->ResolveGeoreference();

  if (!Georeference)
    return;

  UCesiumSubLevelSwitcherComponent* Switcher =
      Georeference->GetSubLevelSwitcher();
  if (!Switcher)
    return;

  const TArray<TWeakObjectPtr<ALevelInstance>>& Sublevels =
      Switcher->GetRegisteredSubLevelsWeak();

  // If we don't have any known sub-levels, and aren't origin shifting outside
  // of sub-levels, then bail quickly to save ourselves a little work.
  if (Sublevels.IsEmpty() &&
      this->Mode != ECesiumOriginShiftMode::ChangeCesiumGeoreference &&
      this->Mode != ECesiumOriginShiftMode::ChangeWorldOriginLocation) {
    return;
  }

  FVector ActorEcef = this->GlobeAnchor->GetEarthCenteredEarthFixedPosition();

  ALevelInstance* ClosestActiveLevel = nullptr;
  double ClosestLevelDistance = std::numeric_limits<double>::max();

  for (int32 i = 0; i < Sublevels.Num(); ++i) {
    ALevelInstance* Current = Sublevels[i].Get();
    if (!IsValid(Current))
      continue;

    UCesiumSubLevelComponent* SubLevelComponent =
        Current->FindComponentByClass<UCesiumSubLevelComponent>();
    if (!IsValid(SubLevelComponent))
      continue;

    if (!SubLevelComponent->GetEnabled())
      continue;

    FVector LevelEcef =
        UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
            FVector(
                SubLevelComponent->GetOriginLongitude(),
                SubLevelComponent->GetOriginLatitude(),
                SubLevelComponent->GetOriginHeight()));

    double LevelDistance = FVector::Distance(LevelEcef, ActorEcef);
    if (LevelDistance < SubLevelComponent->GetLoadRadius() &&
        LevelDistance < ClosestLevelDistance) {
      ClosestActiveLevel = Current;
      ClosestLevelDistance = LevelDistance;
    }
  }

  Switcher->SetTargetSubLevel(ClosestActiveLevel);

  bool doOriginShift =
      Switcher->GetTargetSubLevel() == nullptr &&
      Switcher->GetCurrentSubLevel() == nullptr &&
      this->Mode != ECesiumOriginShiftMode::SwitchSubLevelsOnly;

  if (doOriginShift) {
    AActor* Actor = this->GetOwner();
    doOriginShift =
        IsValid(Actor) && Actor->GetActorLocation().SquaredLength() >
                              this->Distance * this->Distance;
  }

  if (doOriginShift) {
    if (this->Mode == ECesiumOriginShiftMode::ChangeCesiumGeoreference) {
      Georeference->SetOriginEarthCenteredEarthFixed(ActorEcef);
    } else if (
        this->Mode == ECesiumOriginShiftMode::ChangeWorldOriginLocation) {
      UWorld* World = GetWorld();
      AActor* Actor = this->GetOwner();
      if (IsValid(World) && IsValid(Actor)) {
        const FIntVector& OriginLocation = World->OriginLocation;
        FVector WorldPosition = Actor->GetActorLocation();
        FIntVector WorldPositionInt(
            int32(WorldPosition.X),
            int32(WorldPosition.Y),
            int32(WorldPosition.Z));
        int32 X = clampedAdd(OriginLocation.X, WorldPositionInt.X);
        int32 Y = clampedAdd(OriginLocation.Y, WorldPositionInt.Y);
        int32 Z = clampedAdd(OriginLocation.Z, WorldPositionInt.Z);
        FIntVector NewOriginLocation(X, Y, Z);
        if (NewOriginLocation != OriginLocation) {
          World->SetNewWorldOrigin(NewOriginLocation);
        }
      }
    }
  }
}

void UCesiumOriginShiftComponent::ResolveGlobeAnchor() {
  this->GlobeAnchor = nullptr;

  AActor* Owner = this->GetOwner();
  if (!IsValid(Owner))
    return;

  this->GlobeAnchor =
      Owner->FindComponentByClass<UCesiumGlobeAnchorComponent>();
  if (!IsValid(this->GlobeAnchor)) {
    // A globe anchor is missing and required, so add one.
    this->GlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(Owner->AddComponentByClass(
            UCesiumGlobeAnchorComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    Owner->AddInstanceComponent(this->GlobeAnchor);

    // Force the Editor to refresh to show the newly-added component
#if WITH_EDITOR
    Owner->Modify();
    if (Owner->IsSelectedInEditor()) {
      GEditor->SelectActor(Owner, true, true, true, true);
    }
#endif
  }
}
