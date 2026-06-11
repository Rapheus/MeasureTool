#include "MeasureActor.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

AMeasureActor::AMeasureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorHiddenInGame(true);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	LineMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LineMesh"));
	LineMesh->SetupAttachment(RootComponent);
	LineMesh->bUseComplexAsSimpleCollision = false;
	LineMesh->SetCastShadow(false);
	LineMesh->SetVisibility(false);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetWorldSize(50.0f);
	LabelText->HorizontalAlignment = EHorizTextAligment::EHTA_Center;
	LabelText->SetCastShadow(false);
	LabelText->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMat(
		TEXT("/MeasureTool/Materials/M_Emissive"));
	if (DefaultMat.Succeeded())
	{
		LineMesh->SetMaterial(0, DefaultMat.Object);
		LineMaterial = DefaultMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CameraFacingMat(
		TEXT("/MeasureTool/Materials/M_CameraFacing"));
	if (CameraFacingMat.Succeeded())
		LabelText->SetMaterial(0, CameraFacingMat.Object);
}

void AMeasureActor::PostLoad()
{
	Super::PostLoad();
	// OverrideMaterials[0] serialises as null (LineMID is Transient).
	// Re-apply the base material so the mesh never renders with a null slot.
	if (LineMaterial && LineMesh)
		LineMesh->SetMaterial(0, LineMaterial);
}

void AMeasureActor::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	// Same as PostLoad: OverrideMaterials[0] on the duplicated mesh holds a live
	// cross-actor pointer to the original's transient LineMID. Reset it before
	// RegisterAllComponents creates the render proxy so there is no material flash.
	LineMID             = nullptr;
	bMeshSectionCreated = false;
	if (LineMaterial && LineMesh)
		LineMesh->SetMaterial(0, LineMaterial);
}

void AMeasureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Reset LineMID and normalise the slot so UpdateLine always creates a fresh MID
	// from LineMaterial. This is the authoritative fix for post-duplication stale state.
	LineMID = nullptr;
	if (LineMaterial && LineMesh)
		LineMesh->SetMaterial(0, LineMaterial);
	UpdateMeasurement(true);
}

void AMeasureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateMeasurement(false);
}

void AMeasureActor::CreateAnchor()
{
	if (AnchorA && AnchorB) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const bool   bIsA   = !AnchorA;
	const TCHAR* Suffix = bIsA ? TEXT("A") : TEXT("B");

	// ATargetPoint has a billboard sprite and arrow in the editor viewport.
	// Load by path to avoid a header dependency; cache the class across calls.
	static UClass* AnchorClass = StaticLoadClass(AActor::StaticClass(), nullptr,
		TEXT("/Script/Engine.TargetPoint"));
	if (!AnchorClass)
		AnchorClass = AActor::StaticClass();

	FActorSpawnParameters Params;
	Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	AActor* NewAnchor = World->SpawnActor<AActor>(
		AnchorClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		Params
	);
	if (!NewAnchor) return;

	// ATargetPoint sets its root in editor builds; guard the plain-AActor fallback.
	if (!NewAnchor->GetRootComponent())
	{
		USceneComponent* AnchorRoot = NewObject<USceneComponent>(
			NewAnchor, USceneComponent::StaticClass(), TEXT("DefaultSceneRoot"));
		NewAnchor->SetRootComponent(AnchorRoot);
		AnchorRoot->RegisterComponent();
		NewAnchor->SetActorLocation(GetActorLocation());
	}

	// Ensure the root is moveable so SetActorLocation works in editor ticks.
	if (USceneComponent* AnchorRoot = NewAnchor->GetRootComponent())
		AnchorRoot->SetMobility(EComponentMobility::Movable);

	// Attach to this actor so anchors live in the hierarchy by default.
	// Users can detach them in the outliner if they need independent actors.
	NewAnchor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

#if WITH_EDITOR
	NewAnchor->SetActorLabel(FString::Printf(TEXT("%s_Anchor%s"), *GetActorLabel(), Suffix));
#endif

	if (bIsA)
		AnchorA = NewAnchor;
	else
		AnchorB = NewAnchor;
}

void AMeasureActor::SetPivotToAnchorA()
{
	if (!AnchorA) return;

	// Capture both anchors' world transforms up front. Attached anchors follow
	// the actor when we move its pivot; detached ones don't. Restoring the saved
	// world transforms afterwards keeps every anchor fixed in world space either way.
	const FTransform WorldA = AnchorA->GetActorTransform();
	const bool       bHasB  = (AnchorB != nullptr);
	const FTransform WorldB = bHasB ? AnchorB->GetActorTransform() : FTransform::Identity;

	SetActorLocation(WorldA.GetLocation());

	AnchorA->SetActorTransform(WorldA);
	if (bHasB)
		AnchorB->SetActorTransform(WorldB);

	UpdateMeasurement(true);
}

void AMeasureActor::UpdateMeasurement(bool bForce)
{
	if (!AnchorA || !AnchorB)
	{
		LineMesh->SetVisibility(false);
		LabelText->SetVisibility(false);
		return;
	}

	const FVector PosA = AnchorA->GetActorLocation();
	const FVector PosB = AnchorB->GetActorLocation();
	DistanceCM = FVector::Dist(PosA, PosB);

	UpdateLine(PosA, PosB);

	if (bShowLabel)
	{
		const FVector LabelPos = FMath::Lerp(PosA, PosB, LabelPosition);
		LabelText->SetWorldLocation(LabelPos);
		LabelText->SetWorldRotation(FRotator::ZeroRotator);
	}

	const bool bLabelChanged = bForce
		|| !FMath::IsNearlyEqual(DistanceCM, LastDistanceCM, 0.001)
		|| bShowLabel      != bLastShowLabel
		|| !FMath::IsNearlyEqual(LabelPosition, LastLabelPosition)
		|| DisplayUnit     != LastDisplayUnit
		|| LabelColor      != LastLabelColor
		|| LabelDecimals   != LastLabelDecimals;

	if (bLabelChanged)
	{
		UpdateLabel();
		LastDistanceCM    = DistanceCM;
		bLastShowLabel    = bShowLabel;
		LastLabelPosition = LabelPosition;
		LastDisplayUnit   = DisplayUnit;
		LastLabelColor    = LabelColor;
		LastLabelDecimals = LabelDecimals;
	}
}

static float GetUnitIntervalCM(EMeasureUnit Unit)
{
	switch (Unit)
	{
	case EMeasureUnit::Centimeter: return 1.0f;
	case EMeasureUnit::Meter:      return 100.0f;
	case EMeasureUnit::Kilometer:  return 100000.0f;
	case EMeasureUnit::Inch:       return 2.54f;
	case EMeasureUnit::Foot:       return 30.48f;
	case EMeasureUnit::Yard:       return 91.44f;
	case EMeasureUnit::Mile:       return 160934.4f;
	default:                       return 1.0f;
	}
}

void AMeasureActor::UpdateLine(const FVector& PosA, const FVector& PosB)
{
	if (DistanceCM < KINDA_SMALL_NUMBER)
	{
		LineMesh->SetVisibility(false);
		return;
	}

	LineMesh->SetVisibility(true);

	const FVector LineDir     = (PosB - PosA).GetSafeNormal();
	const float   TickHalfLen = FMath::Max(TickLength, LineWidth) * 0.5f;

	// Billboard: width direction = cross(lineDir, toCam), gives consistent apparent
	// thickness from any viewing angle.
	FVector CamPos = PosA + FVector(0.f, 0.f, 10000.f);
	const UWorld* World = GetWorld();
	if (World && World->ViewLocationsRenderedLastFrame.Num() > 0)
		CamPos = World->ViewLocationsRenderedLastFrame[0];

	const FVector ToCam    = (CamPos - (PosA + PosB) * 0.5f).GetSafeNormal();
	FVector       WidthDir = FVector::CrossProduct(LineDir, ToCam).GetSafeNormal();
	if (WidthDir.IsNearlyZero())
	{
		WidthDir = FVector::CrossProduct(LineDir, FVector::UpVector).GetSafeNormal();
		if (WidthDir.IsNearlyZero())
			WidthDir = FVector::CrossProduct(LineDir, FVector::ForwardVector).GetSafeNormal();
	}

	// Vertices are in LineMesh component-local space (relative to actor origin).
	// Inverse-rotate world positions and directions into local space to handle
	// any actor rotation correctly.
	const FVector ActorPos  = GetActorLocation();
	const FQuat   InvRot    = GetActorQuat().Inverse();
	const FVector LocalPosA = InvRot.RotateVector(PosA - ActorPos);
	const FVector LocalPosB = InvRot.RotateVector(PosB - ActorPos);
	const FVector LocalW    = InvRot.RotateVector(TickHalfLen * WidthDir);
	const FVector LocalN    = InvRot.RotateVector(ToCam);
	const FVector LocalWDir = InvRot.RotateVector(WidthDir);

	// UVs: U = CM along the line (0 at PosA, DistanceCM at PosB)
	//      V = CM from centre (negative on one side, positive on the other)
	TArray<FVector>          Vertices;
	TArray<FVector2D>        UVs;
	TArray<FVector>          Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32>            Triangles;

	Vertices.SetNum(4);
	UVs.SetNum(4);
	Normals.Init(LocalN, 4);
	Tangents.Init(FProcMeshTangent(LocalWDir, false), 4);

	Vertices[0] = LocalPosA + LocalW;
	Vertices[1] = LocalPosA - LocalW;
	Vertices[2] = LocalPosB + LocalW;
	Vertices[3] = LocalPosB - LocalW;

	UVs[0] = FVector2D(0.f,        +TickHalfLen);
	UVs[1] = FVector2D(0.f,        -TickHalfLen);
	UVs[2] = FVector2D(DistanceCM, +TickHalfLen);
	UVs[3] = FVector2D(DistanceCM, -TickHalfLen);

	Triangles = { 0, 2, 1,  1, 2, 3 };

	if (!bMeshSectionCreated)
	{
		LineMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs,
		                            TArray<FColor>(), Tangents, false);
		bMeshSectionCreated = true;
	}
	else
	{
		LineMesh->UpdateMeshSection(0, Vertices, Normals, UVs,
		                            TArray<FColor>(), Tangents);
	}

	if (!LineMID && LineMaterial)
	{
		LineMID = UMaterialInstanceDynamic::Create(LineMaterial, this);
		if (LineMID)
			LineMesh->SetMaterial(0, LineMID);
	}

	if (LineMID)
	{
		const float SafeUnitCM = FMath::Max(GetUnitIntervalCM(DisplayUnit), KINDA_SMALL_NUMBER);
		LineMID->SetScalarParameterValue(TEXT("UnitIntervalCM"),    SafeUnitCM);
		LineMID->SetScalarParameterValue(TEXT("LineHalfWidthCM"),  LineWidth * 0.5f);
		LineMID->SetScalarParameterValue(TEXT("TickHalfWidthCM"),  TickWidth * 0.5f);
		LineMID->SetScalarParameterValue(TEXT("TickHalfLengthCM"), TickLength * 0.5f);
		LineMID->SetScalarParameterValue(TEXT("ShowTicks"),        bShowTicks ? 1.f : 0.f);
		LineMID->SetScalarParameterValue(TEXT("LineLengthCM"),    (float)DistanceCM);
		LineMID->SetVectorParameterValue(TEXT("Color"),            LineColor);
	}
}

void AMeasureActor::UpdateLabel()
{
	LabelText->SetVisibility(bShowLabel);
	if (!bShowLabel) return;

	LabelText->SetText(FText::FromString(FormatDistance()));
	LabelText->SetTextRenderColor(LabelColor.ToFColor(true));
}

FString AMeasureActor::FormatDistance() const
{
	double Value = DistanceCM;
	const TCHAR* Unit = TEXT("cm");

	switch (DisplayUnit)
	{
	case EMeasureUnit::Centimeter:                         Unit = TEXT("cm"); break;
	case EMeasureUnit::Meter:      Value /= 100.0;          Unit = TEXT("m");  break;
	case EMeasureUnit::Kilometer:  Value /= 100000.0;       Unit = TEXT("km"); break;
	case EMeasureUnit::Inch:       Value /= 2.54;           Unit = TEXT("in"); break;
	case EMeasureUnit::Foot:       Value /= 30.48;          Unit = TEXT("ft"); break;
	case EMeasureUnit::Yard:       Value /= 91.44;          Unit = TEXT("yd"); break;
	case EMeasureUnit::Mile:       Value /= 160934.4;       Unit = TEXT("mi"); break;
	}

	return FString::Printf(TEXT("%.*f %s"), FMath::Max(LabelDecimals, 0), Value, Unit);
}
