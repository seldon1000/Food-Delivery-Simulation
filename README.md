# Food Delivery Simulation
A simple client/server program that simulates a food delivery system in C.

*This project was assigned as part of the Computer Networks exam at the University of Naples "Parthenope".*

*Please, make sure to read the notes at the end of the README before using the project.*



The program includes four entities, which communicate with each other using TCP connections. Every message sent is either a character or a string.
The idea is that a client can choose which restaurant order from and then make the order. The order is sent to the chosen restaurant, which then submits it to its riders. A rider can choose to accept or refuse the order; the first rider who accepts will actually deliver the order.
Every message needs to pass through the server in order to reach its destination.


# Usage
Use gcc to compile the source code. "server.c" and "restaurant.c" will need '-lpthread' to compile.

To run the program:
- run the server;
- run at least one restaurant;
- run clients and riders as you like.


# NOTES
The program works only on Linux machines. The project will work in local or on multiple computers connected to the same network. Errors and faults are not handled. The terminal output and comments within the code are in italian. The are some bugs due to the usage of threads: the project could work unproperly on low end systems.


# Special Contributors/Group Members
- Nicolas Mariniello (me) (https://github.com/seldon1000)
- Fabio Friano
