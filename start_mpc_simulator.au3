#include <File.au3>
#include <ColorConstants.au3>
#include <EditConstants.au3>
#include <WindowsConstants.au3>

; The path to the term 2 simulator
global $simulatorPath = @HomePath&"\Desktop\term2_sim_windows\term2_sim.exe"
; The window title of the term 2 simulator
global $window = "self_driving_car_nanodegree_program"

; Close the Term 2 simulator
Func close_simulator()
WinClose ( $window )
EndFunc

; Start the Term 2 simulator in PID mode
; Assume that screen resolution 1280x960 is set
; The windowed mode will be chosen if not already set.
Func simulator()
close_simulator()

Run($simulatorPath)
WinWaitActive("[CLASS:#32770]")

$isChecked = ControlCommand($window,"", "[CLASS:Button; INSTANCE:1]", "IsChecked")
If not $isChecked Then
   ControlClick($window,"", "[CLASS:Button; INSTANCE:1]")
EndIf

ControlClick($window,"", "[CLASS:Button; INSTANCE:3]")
Sleep(10000)
WinMove ( $window,"", 0,0)
#WinWaitActive("[CLASS:UnityWndClass]")
MouseClick ( "" , 74, 265 )  ; previous
Sleep(100)
MouseClick ( "" , 258, 335 ) ; select

; TODO: This is to test if we could reuse the window instead of restarting
;Sleep(10000)
;#WinSetState("[CLASS:UnityWndClass]", "", @SW_SHOW)
;#WinWaitActive("[CLASS:UnityWndClass]")
;#Send("{ESCAPE}")
;#MouseClick ( "" , 74, 265 )
;#Sleep(100)
;#MouseClick ( "" , 74, 265 )
;#Sleep(100)
;#MouseClick ( "" , 258, 335 )

EndFunc


simulator()

