#MOUSE HOOK

This project focuses on mouse event logic and does not focus on creating a visually appealing user interface.

##Functions
- When you press the "Start" button, the application displays the history of mouse movements.
- Left mouse click on the panel is represented by a circle on the image, and the click coordinates are displayed in the list.
- A right-click on a panel is represented by a square on the image, and the click coordinates are displayed in a list.
- Holding down the mouse button and moving the mouse creates a thick line that allows you to track mouse events.

## Usage
1. Click the "Start MS LL" button to activate the mouse hook and start tracking mouse events.
2. Mouse movements and button presses will be displayed in the list and displayed visually on the image.
3. Left mouse clicks are represented by circles and right mouse clicks are represented by squares.
4. While holding down the mouse button, move the mouse to create bold lines on the image.
5. To stop tracking mouse events, click the "MS LL stop" button.

## Code review
The code contains the following main components:

- `WndProc`: handles messages sent to the application's main window and defines its behavior.
- `StartMsHookLL`: activates a low-level mouse hook to listen for mouse events.
- `StopMsHookLL`: Stops the low-level mouse hook.
- `MsHookProcLL`: intercepts and processes mouse messages, displaying user actions on the image and in the list.

## Requirements
- Windows operating system.

## Create and run
To build and run the project, follow these steps:
1. Compile the source code with the appropriate compiler.
2. Run the resulting binary to run the application.
3. Use the user interface to interact with mouse events and view the results.

## Note
This project was developed using the Windows API and was inspired by the need to track and render mouse events. Special thanks to the developers of libraries and resources used in this project.
