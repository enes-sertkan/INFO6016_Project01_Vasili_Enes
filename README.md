# INFO6016_Project01_Vasili_Enes
A simple Chat Application built with C++ using sockets.

## Group Members
- **Enes Sertkan** (1286651)
- **Vasilii Tsiganov** (1266871)

## Getting Started

1. **Build the Solution**: First, build the solution to compile the application.
2. **Run the Server**: Start the server application. You can run it only once. You can start server by running Chat_App\x64\Debug\ChatServer.exe.
3. **Run Multiple Clients**: Launch multiple client instances simultaneously. This allows you to simulate the same user joining different rooms. You can start clients by running Chat_App\x64\Debug\ChatClient.exe.
4. **Enter Your Name**: When prompted, input your name and press **Enter**.
5. **Join the Main Room**: Upon entering your name, you will automatically be placed in a room called **main**.

You can connect to multiple rooms at once and receive messages from all of them simultaneously. When sending a message, it will only be sent to the room you have currently selected (indicated by the room name at the start of the message).

## Available Commands

Use the following commands to interact with the chat application:

- `/help` - Display a list of available commands.
  
- `/join <room name>` - Join an existing room or create a new one.  
  **Example**: `/join room1`

- `/leave` - Leave the room you are currently in.

- `/switch <room name>` - Switch to another room.

- `/rooms` - View a list of available rooms.

## Notes
- Make sure to follow the command syntax exactly to ensure proper functionality.
