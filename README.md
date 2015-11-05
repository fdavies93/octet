# Project Report 8/11/2015

- Notes

Due to the way that C++ loading functions work, the file path for .csv files will not be read correctly unless you compile the game, then run it from the bin folder of octet. That is, you can't run the game directly through a debugger.

Controls for the game are WASD to move, and spacebar to shoot.

- Introduction and Design

My assignment is a simple top-down shooter modelled primarily on the 2D Legend of Zelda games (particularly the original title for the NES) and the modern game The Binding of Isaac. In the game the player must fight their way through a series of rooms, eliminating all enemies in each room to progress, until they reach the boss. If the player defeats the boss of the game, they win; if they lose all their HP, they lose.

- Project Timeline

I began programming the project on 11/10/2015, and completed it to the assignment specification on 05/11/2015.

For most of the first two weeks of this project, I examined the code of example_invaderers to determine some of its structure and inner workings, particularly those related to rendering, and annotated the parts of the code of this example which I did not understand, particularly those related to OpenGL. During the same period, I brainstormed ideas for the genre and gameplay of the game, and began to think about how I would implement my design ideas. 

- Program Structure

In past game projects, I have found it extremely productive to use a single 'manager' object to handle the higher-level functionality of the game, so it was natural for my first design decision to be to create a manager object of this type. 

+ CSV format

- What I Did Well

- Criticisms of the Project

* OVER-ENGINEERING
* COLLISION SYSTEM
* SHITTY AI

- Places to improve / things learned

- Final reflections