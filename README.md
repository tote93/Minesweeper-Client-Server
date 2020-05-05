# Design and Implementation
Design and implementation of the game of the minesweeper for two players allowing network play. The game is to be the first to find ten mines.

# Main Objectives
To play the minesweeper, you need a grid that initially appears hidden. Once a grid is selected, it indicates a number that represents the mines that are in contact with that box, or a mine. Each square can touch a maximum of 8 mines. The player can place a flag in a cell to indicate that there is a mine, or select it to discover what it contains.
Each player has a turn of play that will alternate. To do this, you will indicate the action you want to take (discover a cell or put a flag if you suspect that a box is hiding a mine) and the coordinates of the cell (A1, A2, ...).
Upon discovering a cell, the server will check the information for that cell:
- If the cell has a mine, it will send a message to both players indicating that the player who has discovered the cell has lost, and the game will end.
- If the selected square contains an empty cell, all adjacent squares will be discovered until they are surrounded by numbers, the new board will be sent to both players.

The game ends when one of the two players reaches the ten selected mines (he has indicated ten flags on the board), at that moment the server will check if they are correct, in which case it will indicate that said player has won the game, or in Otherwise, it will indicate that you have lost the game. Each player has exclusively ten flags that they can place and cannot move once they are placed, the same cell can have a flag for each of the players.

# Features
The board will always be 10x10, and a total of 20 bombs will be included, which will be randomly placed.
Communication between clients of the minesweeper game will be carried out under the TCP transport protocol. 
The proposed practice consists of creating a client/server application that implements the game with the restrictions mentioned above. In the game in question, the players (the clients) connect to the service (the server). Only games with two players will be accepted, and a maximum of 10 simultaneous games will be accepted. The protocol to be followed will be as follows:
- A client connects to the service, and if the connection was successful, the system
returns "+ 0k. Connected user".
- In order to access the services, it is necessary to identify yourself by sending the
username and password for the system to validate it 1. The messages to be indicated are: "USER user" to indicate the user, after which the server will send "+ Ok. Correct user" or "–Err. Wrong user". In case of being correct, the next message that is expected to be received is "PASSWORD password", where the server will respond with the message "+ Ok. Validated user" or "–Err. Validation failed".
- A new user will be able to register with the message "REGISTRATION –u user –p password." Control will be carried out to avoid collisions with the names of existing users.
- Once connected and validated, the client will be able to carry out a game in the game, indicating a "START-GAME" message. Received this message on the server, it will be in charge of checking the people it has pending to start a game:
    - If, with this request, a pair is already formed, it will send a message to both, to indicate that the game is going to start.
    - If no one was waiting, it would send a message to the new user, specifying that they have their request and that they are waiting for the connection of another player. The user, in this case, must wait to receive a message to play, or they will also be allowed to exit the application.
- When two users are available, a message is left to both of them, indicating that the game will begin "+ Ok. The game begins, "and then information is sent from the game board.

### Each user during his turn:
- You can send the message to DISCOVER letter, number. Before this
the message, if in that position there was no mine, the server will send the player the board again, indicating the new situation, which will reflect the numbers of adjacent cells that have a mine.
    - In this way, both users will be informed of all movements, and the turn passes to the next player. If there was a mine in that position, the server would send a message to each player, indicating that the player who has discovered a mine has lost the game, and that game will end.
- You can send the message PUT-FLAG letter, number. Given this message, the server will send the player the board again, indicating with an "A" or "B," depending on the corresponding player, that there may be a mine in said cell. The turn passes to the next player.
- Care must be taken on the server when user requests arrive when it is not your turn to play. In case they send a message when it's not
their turn, they must answer that it is not yet their turn, and they must wait. The
specification of the message to be sent would be: "-Err. It would help if you waited your turn. "
- In case a user has placed their ten flags, the game ends, the server checks if they are correct, and both users receive the message: "+ Ok. <User name> has won/lost", depending on whether you have hit the mines or not.
- To exit the service, the message "EXIT" will be used; in this way, the server will remove it from connected clients. If it were playing, the game it was in would end, warning the other player: "+ Ok. Your opponent has finished the game", if you were waiting for a game, removes it from the wait.
- Any message that does not use one of the detailed specifiers will generate a Err" message from the server.


# Commands to play
- USER user: message to enter the user you want.
- PASSWORD password: message to enter the password associated with the user.
- REGISTRATION –u user –p password: message by which the user requests to register to access the roulette game he hears on TCP port 2050. o START-GAME: message to request to play a game of sinking the fleet.
- DISCOVER letter, a number where the letter indicates the column, and number the row, of the cell you want to discover.
- PUT-FLAG letter, a number where the letter indicates the column, and number the row, of the cell where you want to place the flag.
- EXIT: message to request to exit the game.
- Any other type of message sent to the server will not be recognized by the protocol as a valid message and will generate its corresponding “-Err.” by the server.
