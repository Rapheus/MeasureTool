#pragma once

#include "CoreMinimal.h"
#include "MeasureToolTypes.generated.h"

UENUM(BlueprintType)
enum class EMeasureUnit : uint8
{
	Centimeter UMETA(DisplayName = "Centimeters"),
	Meter      UMETA(DisplayName = "Meters"),
	Kilometer  UMETA(DisplayName = "Kilometers"),
	Inch       UMETA(DisplayName = "Inches"),
	Foot       UMETA(DisplayName = "Feet"),
	Yard       UMETA(DisplayName = "Yards"),
	Mile       UMETA(DisplayName = "Miles")
};
