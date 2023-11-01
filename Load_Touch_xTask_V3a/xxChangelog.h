/* Changelog
V1.3 - March 2023
General
- Re-calibration is required after V1.3 changes, as the software version number has changed due to additional items being stored in the EEPROM profile.
????????- Added OTA tab to web screen to make software updates easier.
- Updated various screen error message to improve formatting(noted by CW).
- "W" indicator changes to "AP" when WiFi acting as base station.
- fixed "noSetval" bug that caused random reboots.
- added screen rotation button to CAL screen (as well as touch screen rotate)
- maximum logging interval extended to 999.999 seconds
- improved code to keep digit selection highlight in view when a different value setting is selected
***- Separated CPU allocation for main loop (CPU0) and events (CPU1) to fix occasional lost interrupts from ADS ADC
- miniumm resistance setting for CR mode increased from 0.11 to 0.2 ohms.
- resistance and power values display as 0.00 when OFF
- changed warning message blocks to non-blocking code and added screen hold for critical messages.
- Voltage and current now auto-zero (Kelvin mod is required), CAL menu DEF button added: reset all calibrations to factory defaults.
- Updated steps min/max settings.
- Screen reverts to Main when ON is pressed.
- Menu and submenu bottons are now orange.

Battery testing (updated with code from EB)
- several voltage tests now performed before ending battery test cycle.
- increased log interval to 20s for battery testing to increase test time before log buffer is overwritten with new data.
- final battery test results now persist until user interacts with screen.
- Battery test mAH now resets on initiation of new test cycle.
- system restart after BT completion fixed.
- LI value on main screen now used for intervals in battery test cycle. LI == 0 does not log any values other than at the end of the test.
- increased 
- addded elapsed test time log
- changes to settings only updated each log interval (when non-zero)

Control Task
- extensive improvements to current limiting for CP, CR, CV modes.
-- CV mode disconnects when over current limit.
- Moved process and start next ADS conversion to controlTask() to avoid ADS processing delays.
- Improved DAC setting estimate for substantial current changes (> 0.5A) in CC mode.
- code changes to improve processing of ESP and ADS ADC readings.
-- bug in controlTask checking for difference in ESP/ADS ADC amps readings corrected
- Increased ESP32 ADC multisampling to improve accuracy of ESP32 V & I measurements.
-- excluded  high and low samples from multisampling average 
-- Point at which ESP reading current difference from ADC triggers immediate control response increased from 0.4A to 5A (C_VARCOARSE)
- extended operational SOA checking 
-- extended checking for minimum voltage in CP and CR modes
- minimum step duration increased from 5mS to 10mS

Web interface
- corrected bug where max setting current is 15A with daughterboad installed
????????- OTA tab added (as above)

Calibration & measurement
- Main, Kelvin and Auto voltage measurement modes supported in SET menu
- M/K/A incicator at top right of screen.
- Load will not turn ON in Kelvin mode if V is very small or less than Main volts. 
- auto calibration improved
- CAL process changed

Testing
- Sketch with stepped DAC settings added to repository for testing output stages. 
- Sketch with stepped DAC settings plus ESP32 and ADS ADC readings added for testing.

Compile and upload
- full code added to repository with notes on dependencies
- "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)" partition scheme recommended to reduce possibility of insufficient memory for program upload.

V1.2 - Initial release, Jan 2022
*/
