#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeasureToolTypes.h"
#include "MeasureActor.generated.h"

class UProceduralMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(Abstract, HideCategories = (Collision, Replication, Actor, Input, Cooking))
class MEASURETOOL_API AMeasureActor : public AActor
{
	GENERATED_BODY()

public:
	AMeasureActor();

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	// -------------------------------------------------------------------------
	// Anchors
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Measure")
	TObjectPtr<AActor> AnchorA;

	UPROPERTY(EditAnywhere, Category = "Measure")
	TObjectPtr<AActor> AnchorB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Measure")
	double DistanceCM = 0.0;

	// Spawns a plain AActor at this actor's location and assigns it to
	// AnchorA first, then AnchorB. No-op when both slots are filled.
	UFUNCTION(meta = (CallInEditor = "true"), Category = "Measure")
	void CreateAnchor();

	// Moves this actor's pivot to AnchorA's world location, then restores both
	// anchors' world transforms so they stay visually fixed in world space.
	UFUNCTION(meta = (CallInEditor = "true", DisplayName = "Set Pivot To Anchor A"), Category = "Measure")
	void SetPivotToAnchorA();

	// -------------------------------------------------------------------------
	// Line
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Line", meta = (ClampMin = "0.1"))
	float LineWidth = 5.0f;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Line")
	FLinearColor LineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Line")
	bool bShowTicks = true;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Line", meta = (ClampMin = "0.01"))
	float TickWidth = 4.0f;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Line", meta = (ClampMin = "0.1"))
	float TickLength = 25.0f;

	// -------------------------------------------------------------------------
	// Label
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Label")
	bool bShowLabel = true;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Label")
	EMeasureUnit DisplayUnit = EMeasureUnit::Meter;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Label", meta = (ClampMin = "0", ClampMax = "6"))
	int32 LabelDecimals = 2;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Label", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LabelPosition = 0.5f;

	UPROPERTY(EditAnywhere, Interp, Category = "Measure|Label")
	FLinearColor LabelColor = FLinearColor::White;


protected:
	virtual void PostLoad() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> LineMesh;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> LabelText;

	// Label dirty-check cache
	double LastDistanceCM        = -1.0;
	bool  bLastShowLabel         = false;
	float LastLabelPosition      = -1.0f;
	EMeasureUnit LastDisplayUnit = EMeasureUnit::Meter;
	FLinearColor LastLabelColor  = FLinearColor::White;
	int32        LastLabelDecimals = 2;

	bool bMeshSectionCreated = false;

	// Cached base material read from LineMesh slot 0; updated when the slot changes externally.
	UPROPERTY()
	TObjectPtr<UMaterialInterface> LineMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LineMID;

	// Pass bForce = true to bypass label dirty-check (used by OnConstruction).
	void UpdateMeasurement(bool bForce = false);

	void UpdateLine(const FVector& PosA, const FVector& PosB);
	void UpdateLabel();
	FString FormatDistance() const;
};
