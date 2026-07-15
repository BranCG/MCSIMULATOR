#include "MCSIMULATORGameMode.h"
#include "VRCharacter.h"

AMCSIMULATORGameMode::AMCSIMULATORGameMode()
{
	// Set default pawn class to our C++ VR Character class
	DefaultPawnClass = AVRCharacter::StaticClass();
}
