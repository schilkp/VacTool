# VacTool
### Philipp Schilk

A very simple PIC12F1572-based controller for a manual vacuum SMD component pick-and-place pen. 

![PCB Render](https://raw.githubusercontent.com/TheSchilk/VacTool/master/Doc/Render.jpg)

# Description
Allows control of a 12Vdc solenoid valve and air pump using a 1/4" jack foot pedal.  
The solenoid is activated when the pedal is pressed. The operation of the pump is 
controlled by the position of the potentiometer:  

When the potentiometer is fully left, the pump follows the solenoid/foot pedal exactly.  

When the potentiometer is fully right, the pump runs continously.

In between, the pump will run when the solenoid is active, and will keep running for a while 
after the solenoid closes again determined by the potentiometer position. When continously
placing parts this should allow for a smoother workflow.

# Inspiration
Steve Gardner's (aka SDG Electronics) Vaccum Pick and Place Tool:  

[Youtube Video](https://www.youtube.com/watch?v=1FnGqH_WkL4)  

[Website](https://sdgelectronics.co.uk/youtube-videos/a-diy-smd-pick-and-place-tool-for-electronics-assembly/)  


