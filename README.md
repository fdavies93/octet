# Project Report 8/11/2015

## Notes

The file path for .csv files will not be read correctly unless you compile the game, then run it from the bin folder of octet. That is, you can't run the game directly through a debugger.

You can find a gameplay video at https://www.youtube.com/watch?v=8URPZv4AyFQ

Controls for the game are WASD to move, and spacebar to shoot.

## Introduction and Design

My assignment is a simple top-down shooter modelled primarily on the 2D Legend of Zelda games (particularly the original title for the NES) and the modern game The Binding of Isaac. In the game the player must fight their way through a series of rooms, eliminating all enemies in each room to progress, until they reach the boss. If the player defeats the boss of the game, they win; if they lose all their HP, they lose.

All maps are loaded from CSV files found in assets/assignment_nov_8. There is also a custom shader called assignment_shader, which simply blends all sprites into the background colour by mixing that colour with the sprite's texture's colour. 

## Project Timeline

I began programming the project on 11/10/2015, and completed it to the assignment specification on 05/11/2015.

For most of the first two weeks of this project, I examined the code of example_invaderers to determine some of its structure and inner workings, particularly those related to rendering, and annotated the parts of the code of this example which I did not understand, particularly those related to OpenGL. During the same period, I brainstormed ideas for the genre and gameplay of the game, and began to think about how I would implement my design ideas (for implementation details, see the next section).

In week 3 I copied the invaderers code to a new project called assignment_8_nov, produced a high-level manager object, and laid the basic framework for distinguishing between different types of sprite, as well as functions to add and remove sprites generically rather than having to know the memory address of each sprite separately as in invaderers. I also added collision code. At this point the basic engine features were more or less in place.

In week 4 I was extremely busy and did not work on the project.

In week 5 I finalised the features of the engine by adding a basic sound capability and began to work on adding assets and specific gameplay logics. This included code allowing enemies to move around, hit things, and so on, as well as some coding to separate collided objects from one another.

In week 6 (Reading Week) I wrote the code to load maps from CSVs and continued work on the AI of the game. This included making it possible to move between rooms, having maps be connected by information stored in the header of their CSV files, and making win and lose conditions possible. I also shifted the text rendering code from invaderers into my manager object. Finally, I wrote the fragment shader to blend sprites with the background. This week involved one large task (CSVs) and many small tasks related to AI. I also made an attempt to improve the code which separates collided sprites, but this was unsuccessful due to deeper problems with the program.

## Program Structure

In past game projects, I have found it extremely productive to use a single 'manager' object to handle the higher-level functionality of the game, so it was natural for my first design decision to be to create a manager object of this type. This decision to centralise control informed the majority of the rest of my design.

I aimed to keep sprite objects relatively lightweight and generic, with their main distinguishing feature being their type attribute. This was in order that all collision and simulation code could be centralised in the manager object, and to avoid both complex inheritance hierarchies and the memory and work overhead of implementing a component pattern. In a larger project this would not have been a suitable approach, but while it began to become messy towards the end of the project, it was still manageable.

The vast majority of the idiosyncracities of my code can be referred to these attempts to centralise control; those which can't are largely the product of the criticisms I make in my reflections.

### CSV format

The CSV format of maps is as follows: the first line of the file is the header, and contains information on the four exits to the level (which activate when the player leaves the screen in any direction). Each exit is in the format FILENAME, NEW X COORDINATE, NEW Y COORDINATE; the exit directions go LEFT, RIGHT, UP, DOWN. Therefore, the first entry is the filename of the map to go to if the player exits the left side of the screen, the fourth is the filename if the player exits right, and so on.

## Reflections on Project

### What I Did Well

My CSV loading code is fairly robust; this is because I became frustrated at the problems with the way I couldn't simply edit CSVs in Excel. As a result I dedicated more time than I might otherwise have done to this aspect of the project in order to make it work as intended.

While the project has its flaws (see below) it also plays reasonably well -- it has a win and lose condition, and a clear set of logics at work. With a little extra time and work, it could be a simple yet fun game or a basis for a larger project.

### Criticisms of the Project

I delayed starting probably around a week longer than I ought to have done. While this wasn't completely my fault -- I wasn't sure if we were allowed to edit invaderers or whether the assignment had to be written from scratch -- it has led to time management problems further down the line.

My initial creation of the game manager, while not a terrible design decision in itself, was also problematic. I spent some time making sure the engine separated and rendered colliding sprites and background sprites correctly, for example, when in the end, my final game did not require this feature. Likewise, there was no need for functions such as find_sprite_by_index. In general, the manager object suffered somewhat from over-engineering, leading to the below problems due to time management issues.

The collision system of the game, while serviceable, has a number of problems. It is occasionally possible for objects to pass through other objects when they shouldn't, and multiple simultaneous collisions on the same object are processed badly.

While there is an AI on the enemies in the game, this AI is not 'fun' -- it's random and unpredictable in the case of the basic enemies, and simply unfair in the case of the boss. Beating the boss requires exploiting his behaviour in a non-obvious way.

Towards the end of the project, due to time pressure, my code also became less clean; there were many more universal variables added in the last few days of the project, for example, than in the days before.