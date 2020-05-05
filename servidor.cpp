#include "funcionesAuxiliares.hpp"
#include "sala.hpp"
#include "usuario.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <vector>
using namespace std;

#define MSG_SIZE 250
#define MAX_CLIENTS 50

/*
 * El servidor ofrece el servicio de un chat
 */

int main()
{

	/*----------------------------------------------------
	    Descriptor del socket y buffer de datos
	-----------------------------------------------------*/
	int sd, new_sd;
	struct sockaddr_in sockname, from;
	char buffer[MSG_SIZE];
	socklen_t from_len;
	fd_set readfds, auxfds;
	int salida;
	int arrayClientes[MAX_CLIENTS];
	int numClientes = 0;
	//contadores
	int i, j, k;
	int recibidos;
	char identificador[MSG_SIZE];

	int on, ret;

	std::vector<usuario> vector_usuarios;
	std::vector<sala> vector_salas;

	/* --------------------------------------------------
	    Open the socket
	---------------------------------------------------*/
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
		perror("Unable to open client socket\n");
		exit(1);
	}

	// We will activate a socket property that will allow others
	// sockets can reuse any port we bind to.
	// This will allow · in protocols like TCP, to be able to execute a
	// same program several times in a row and always link it to
	// same port. Otherwise you would have to wait for the port
	// remain available (TIME_WAIT in the case of TCP)
	on = 1;
	ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(2000);
	sockname.sin_addr.s_addr = INADDR_ANY;

	if (bind(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1)
	{
		perror("Bind operation failed");
		exit(1);
	}

	/*---------------------------------------------------------------------
	    Of the requests that we are going to accept we only need the
		size of its structure, the rest of the information (family, port,
		ip), will be provided by the method that receives the requests.
	----------------------------------------------------------------------*/
	from_len = sizeof(from);

	if (listen(sd, 1) == -1)
	{
		perror("Listen operation failed");
		exit(1);
	}

	//Initialize fd_set sets
	FD_ZERO(&readfds);
	FD_ZERO(&auxfds);
	FD_SET(sd, &readfds);
	FD_SET(0, &readfds);

	//We capture the SIGINT signal (Ctrl + c)
	signal(SIGINT, manejador);

	/*-----------------------------------------------------------------------
	    The server accepts a request
	------------------------------------------------------------------------ */
	while (1)
	{

		//We hope to receive messages from clients (new connections or messages from clients already connected)

		auxfds = readfds;

		salida = select(FD_SETSIZE, &auxfds, NULL, NULL, NULL);

		if (salida > 0)
		{

			for (i = 0; i < FD_SETSIZE; i++)
			{

				//We look for the socket through which communication has been established
				if (FD_ISSET(i, &auxfds))
				{

					if (i == sd)
					{

						if ((new_sd = accept(sd, (struct sockaddr *)&from, &from_len)) == -1)
						{
							perror("Error accepting requests");
						}
						else
						{
							if (vector_usuarios.size() < MAX_CLIENTS)
							{
								usuario new_user;
								// The new user will be inserted into the array with id = i and status = 0
								new_user.setId(new_sd);
								new_user.setEstado(0);
								vector_usuarios.push_back(new_user);
								int pos = buscar_usuario(vector_usuarios, new_sd);								
								FD_SET(new_sd, &readfds);
								strcpy(buffer, "+OK. User Connected\n");
								send(new_sd, buffer, sizeof(buffer), 0);
							}
							else
							{
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "-ERR. Too many connected clients\n");
								send(new_sd, buffer, sizeof(buffer), 0);
								close(new_sd);
							}
						}
					}
					else if (i == 0)
					{
						// Keyboard information has been entered
						bzero(buffer, sizeof(buffer));
						fgets(buffer, sizeof(buffer), stdin);

						// Check if "EXIT" has been entered, closing all sockets and finally leaving the server.
						if (strcmp(buffer, "EXIT\n") == 0)
						{
							for (int j = 0; j < vector_usuarios.size(); ++j)
							{
								send(vector_usuarios[j].getId(), "-ERR. Server disconnection\n ", sizeof(" - ERR. Server disconnection\n"), 0);
								close(vector_usuarios[j].getId());
								FD_CLR(vector_usuarios[j].getId(), &readfds);
							}
							close(sd);
							exit(-1);
						}
					}
					else
					{
						bzero(buffer, sizeof(buffer));
						int posicion = buscar_usuario(vector_usuarios, i);

						recibidos = recv(i, buffer, sizeof(buffer), 0);
						string buffer_string(buffer); // String conversion
						std::vector<string> v = explode(buffer_string, " ");
						if (recibidos > 0)
						{
							//Find " " ocurrence
							if (buffer_string.find(" ") != string::npos)
							{
								string busqueda = buffer_string.substr(0, buffer_string.find(" "));
								//If you find the word REGISTRATION
								if (busqueda.compare("REGISTRATION") == 0)
								{
									// Divide the string and store it in an array
									// I check past parameters
									if (v[1] == "-u" && v[3] == "-p")
									{
										if (existe_usuario(v[2]))
										{
											//User exists
											bzero(buffer, sizeof(buffer));
											strcpy(buffer, "-ERR. Existing user\n");
											send(i, buffer, sizeof(buffer), 0);
										}
										else
										{
											//User not exists, therefore it must be generated
											generar_usuario(v[2], v[4]);
											bzero(buffer, sizeof(buffer));
											strcpy(buffer, "+OK. Created user\n");
											send(i, buffer, sizeof(buffer), 0);
										}
									}
									else
									{
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, "-ERR. Wrong Parameters");
										send(i, buffer, sizeof(buffer), 0);
									}
								}
								else
								{
									// The string USER has been found
									if (busqueda.compare("USER") == 0)
									{
										// The array is checked for the correct size to avoid violations
										if (v.size() != 2)
										{
											bzero(buffer, sizeof(buffer));
											strcpy(buffer, "-ERR. Invalid inserted command\n");
											send(i, buffer, sizeof(buffer), 0);
										}
										else
										{
											// I take the position of the user from the array, to be able to work with it
											int pos = buscar_usuario(vector_usuarios, i);
											//Si estado es = 0, procedo a operar
											if (vector_usuarios[pos].getEstado() == 0)
											{
												//check if the user doesn't exist and I send an error
												if (!existe_usuario(v[1]))
												{
													bzero(buffer, sizeof(buffer));
													strcpy(buffer, "-ERR. User not found\n");
													send(i, buffer, sizeof(buffer), 0);
												}
												else
												{
													// It has valid status and the user does exist
													if (existe_usuario(v[1]) && buscar_usuario(vector_usuarios, i, v[1]) == 0)
													{
														// User passed if exists in file
														bzero(buffer, sizeof(buffer));
														// get his position
														int posicion = buscar_usuario(vector_usuarios, i);
														// operate with the user
														vector_usuarios[posicion].setNombre(v[1]);
														vector_usuarios[posicion].setEstado(1);
														strcpy(buffer, "+OK. Correct user\n");
														send(i, buffer, sizeof(buffer), 0);
													}
													else
													{
														// In this case the user is already inside the system
														bzero(buffer, sizeof(buffer));
														strcpy(buffer, "-ERR. Already logged in user\n");
														send(i, buffer, sizeof(buffer), 0);
													}
												}
											}
											else
											{
												bzero(buffer, sizeof(buffer));
												strcpy(buffer, "-ERR. Invalid state.");
												send(i, buffer, sizeof(buffer), 0);
											}
										}
									}
									else
									{
										// check if you have entered the password command
										if (busqueda.compare("PASSWORD") == 0)
										{											
											if (v.size() != 2)
											{
												bzero(buffer, sizeof(buffer));
												strcpy(buffer, "-ERR. Invalid inserted command\n");
												send(i, buffer, sizeof(buffer), 0);
											}
											else
											{
												int pos = buscar_usuario(vector_usuarios, i);
												if (vector_usuarios[pos].getEstado() == 1)
												{
													// this case implies that you have already inserted your name
													if (comprobar_password(vector_usuarios[posicion].getNombre(), v[1]))
													{
														// There is the username-password pair, we assign the data to its position
														vector_usuarios[posicion].setEstado(2);
														vector_usuarios[posicion].setPassword(v[1]);
														bzero(buffer, sizeof(buffer));
														strcpy(buffer, "+OK. Validated user.\n");
														send(i, buffer, sizeof(buffer), 0);
													}
													else
													{
														bzero(buffer, sizeof(buffer));
														strcpy(buffer, "-ERR. User is not correct.\n");
														send(i, buffer, sizeof(buffer), 0);
													}
												}
												else
												{
													bzero(buffer, sizeof(buffer));
													strcpy(buffer, "-ERR. Invalid state.");
													send(i, buffer, sizeof(buffer), 0);
												}
											}
										}
										else
										{
											if (busqueda.compare("DISCOVER") == 0)
											{
												string opciones = v[1];
												std::vector<string> vopt = explode_user(opciones, ",");
												if (vopt.size() != 2)
												{
													bzero(buffer, sizeof(buffer));
													strcpy(buffer, "-ERR. Invalid command (Letter, Number).");
													send(i, buffer, sizeof(buffer), 0);
												}
												else
												{
													int pos = buscar_usuario(vector_usuarios, i);
													// It has status 4, it is in game
													if (vector_usuarios[pos].getEstado() == 4)
													{
														//check that vopt [1] is a letter and vopt [2] is a number
														int opt;
														std::istringstream(vopt[1]) >> opt;
														// here it is the player's turn
														string letra = vopt[0];
														int col = changeToNumber(letra);
														//Column
														int fila = opt; //Row
														// get the position of the room in the array
														int pos_sala = buscar_sala(vector_salas, vector_usuarios[pos].getId());
														if ((vopt[0] <= "J" && vopt[0] >= "A") && (opt > -1 && opt < 10))
														{

															if (vector_salas[pos_sala].comprobarCasillas(fila, col))
															{
																if (vector_usuarios[pos].getTurno())
																{

																	// take out the partner object for later use
																	usuario pareja;
																	pareja = getPareja(vector_salas, pos_sala, vector_usuarios[pos].getId());
																	// Control who's turn it is to play
																	int pos_pareja = buscar_usuario(vector_usuarios, pareja.getId());
																	vector_usuarios[pos].setTurno(false);
																	vector_usuarios[pos_pareja].setTurno(true);
																	bzero(buffer, sizeof(buffer));
																	// In case check loss is == 1 (you have chosen a bomb)
																	int retorno = vector_salas[pos_sala].comprobarDerrota(fila, col, vector_usuarios[pos].getBandera());
																	if (retorno == 0)
																	{
																		// End the game having lost the user and won the opponent
																		vector_usuarios[pos].setEstado(2);
																		vector_usuarios[pos_pareja].setEstado(2);
																		salirSala(vector_usuarios[pos].getId(), vector_usuarios, vector_salas, 0);
																		// Message for both players showing the board
																		string salida = "+OK.Board. " + vector_salas[pos_sala].printMatriz(0);
																		strcpy(buffer, salida.c_str());
																		send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
																		send(pareja.getId(), buffer, sizeof(buffer), 0);
																		// Message to the player
																		salida = "+OK. " + vector_usuarios[posicion].getNombre() + " has lost, starts a game again to play.\n";
																		strcpy(buffer, salida.c_str());
																		send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
																		// Message to the adversary
																		salida = "+OK. " + vector_usuarios[pos_pareja].getNombre() + " has won, starts a game again to play.\n";
																		strcpy(buffer, salida.c_str());
																		send(pareja.getId(), buffer, sizeof(buffer), 0);
																	}
																	else
																	{
																		if (retorno == 1)
																		{
																			// The game ends and the data is modified with its subsequent win / lose message
																			vector_usuarios[pos].setEstado(2);
																			vector_usuarios[pos_pareja].setEstado(2);
																			salirSala(vector_usuarios[pos].getId(), vector_usuarios, vector_salas, 0);
																			// Message showing the board
																			string salida = "+OK.Board. " + vector_salas[pos_sala].printMatriz(0);
																			strcpy(buffer, salida.c_str());
																			send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
																			send(pareja.getId(), buffer, sizeof(buffer), 0);
																			// Message to the player's partner
																			salida = "+OK. " + vector_usuarios[pos_pareja].getNombre() + " has lost, , starts a game again to play..\n";
																			strcpy(buffer, salida.c_str());
																			send(vector_usuarios[pos_pareja].getId(), buffer, sizeof(buffer), 0);
																			// Message to the player
																			salida = "+OK. " + vector_usuarios[posicion].getNombre() + " has won, , starts a game again to play..\n";
																			strcpy(buffer, salida.c_str());
																			send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																		}
																		else
																		{
																			// This option implies that they continue playing and shows the board to both players
																			vector_salas[pos_sala].buscarMinas(fila, col);
																			string salida = "+OK.Board. " + vector_salas[pos_sala].printMatriz(0);
																			strcpy(buffer, salida.c_str());
																			send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
																			send(pareja.getId(), buffer, sizeof(buffer), 0);
																		}
																	}
																}
																else
																{
																	bzero(buffer, sizeof(buffer));
																	strcpy(buffer, "-ERR. It's not your turn.\n");
																	send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																}
															}
															else
															{
																bzero(buffer, sizeof(buffer));
																strcpy(buffer, "-ERR. Cell already explored.\n");
																send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
															}
														}
														else
														{
															bzero(buffer, sizeof(buffer));
															strcpy(buffer, "-ERR. Invalid parameters (Letter Number).\n");
															send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
														}
													}
													else
													{
														bzero(buffer, sizeof(buffer));
														strcpy(buffer, "-ERR. Invalid state.");
														send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
													}
												}
											}
											else
											{
												if (buffer_string.find("PUT-FLAG") != string::npos)
												{
													// We look for the user's position in the array
													int posicion = buscar_usuario(vector_usuarios, i);
													// We check the status of the player
													if (vector_usuarios[posicion].getEstado() < 4)
													{
														bzero(buffer, sizeof(buffer));
														strcpy(buffer, "-ERR. Invalid state.");
														send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
													}
													else
													{
														string opciones = v[1];
														// We divide the array by the comma and check the size to avoid violations
														std::vector<string> vopt = explode_user(opciones, ",");
														if (vopt.size() != 2)
														{
															bzero(buffer, sizeof(buffer));
															strcpy(buffer, "-ERR. The command is not correct (Letter Number).");
															send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
														}
														else
														{															
															int opt;
															std::istringstream(vopt[1]) >> opt;
															// We save the letter that the player has inserted and change it to integer
															string letra = vopt[0];
															int col = changeToNumber(letra); //Column
															int fila = opt;					 //Row
															// We remove the position of the user who plays and look for the position of the room for editing
															int pos = buscar_usuario(vector_usuarios, i);
															int pos_sala = buscar_sala(vector_salas, vector_usuarios[pos].getId());
															// I take out the partner object for later use
															usuario pareja;
															pareja = getPareja(vector_salas, pos_sala, vector_usuarios[pos].getId());
															// I control who's turn it is to play
															int pos_pareja = buscar_usuario(vector_usuarios, pareja.getId());
															if ((vopt[0] <= "J" && vopt[0] >= "A") && (opt > -1 && opt < 10))
															{

																if (vector_salas[pos_sala].comprobarCasillas(fila, col))
																{
																	// If the player has flags available, he can play
																	if (vector_usuarios[pos].getNumBanderas() - 1 >= 0)
																	{
																		// Put a flag on the board and decrease the player's flags
																		vector_salas[pos_sala].ponerBandera(fila, col, vector_usuarios[pos].getBandera());
																		vector_usuarios[pos].setNumBanderas(vector_usuarios[pos].getNumBanderas() - 1);
																		// We check the flags to indicate that you have won or lost (when you have spent any of your 10 flags)
																		if (vector_salas[pos_sala].comprobarBanderas(vector_usuarios[pos].getBandera()) && vector_usuarios[pos].getNumBanderas() == 0)
																		{
																			bzero(buffer, sizeof(buffer));
																			vector_usuarios[posicion].setEstado(3);
																			vector_usuarios[pos_pareja].setEstado(3);
																			// We modify their states and send a msg to both indicating that p1 has won and p2 lost
																			strcpy(buffer, "+OK. You won, start a game again to play.\n");
																			send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																			strcpy(buffer, "+OK. You lost, start a game again to play.\n");
																			send(vector_usuarios[pos_pareja].getId(), buffer, sizeof(buffer), 0);
																		}
																		else
																		{
																			//It implies that you have won j2 and lost j1
																			if (!vector_salas[pos_sala].comprobarBanderas(vector_usuarios[pos].getBandera()) && vector_usuarios[pos].getNumBanderas() == 0)
																			{
																				bzero(buffer, sizeof(buffer));
																				strcpy(buffer, "+OK. You lost, start a game again to play.\n");
																				send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																				strcpy(buffer, "+OK. You won, start a game again to play.\n");
																				send(vector_usuarios[pos_pareja].getId(), buffer, sizeof(buffer), 0);
																			}
																			else
																			{
																				//If the opponent has not spent all the flags, continue playing and modifying turns
																				usuario pareja = getPareja(vector_salas, pos_sala, vector_usuarios[pos].getId());
																				int pos_pareja = buscar_usuario(vector_usuarios, pareja.getId());
																				vector_usuarios[pos].setTurno(false);
																				vector_usuarios[pos_pareja].setTurno(true);
																				//Msg for both players
																				string salida = "+OK.Board. " + vector_salas[pos_sala].printMatriz(0);
																				strcpy(buffer, salida.c_str());
																				send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
																				send(pareja.getId(), buffer, sizeof(buffer), 0);
																			}
																		}
																	}
																	else
																	{
																		bzero(buffer, sizeof(buffer));
																		strcpy(buffer, "-ERR. Limit of flags reached.\n");
																		send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																	}
																}
																else
																{
																	bzero(buffer, sizeof(buffer));
																	strcpy(buffer, "-ERR. Cell already explored.\n");
																	send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
																}
															}
															else
															{
																bzero(buffer, sizeof(buffer));
																strcpy(buffer, "-ERR. Invalid parameters (Letter Number).\n");
																send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
															}
														}
													}
												}
												else
												{
													bzero(buffer, sizeof(buffer));
													strcpy(buffer, "-ERR. Invalid command.");
													send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
												}
											}
										}
									}
								}
							}
							else
							{
								if (buffer_string.find("START-GAME") != string::npos)
								{

									int posicion = buscar_usuario(vector_usuarios, i);
									if (vector_usuarios[posicion].getEstado() < 2)
									{
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, "-ERR. Invalid state.");
										send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
									}
									else
									{
										if (vector_usuarios[posicion].getEstado() == 2)
										{
											// we remove the position of the user and change the status
											int pos = buscar_usuario(vector_usuarios, i);
											vector_usuarios[pos].setEstado(3);
											// We look for a partner for the user, in case the id is> -1 (there is a partner)
											usuario pareja;
											pareja = buscar_pareja(vector_usuarios, i);

											if (pareja.getId() > -1)
											{
												// We obtain the position of the couple in the users array and modify
												int pos_pareja = buscar_usuario(vector_usuarios, pareja.getId());
												vector_usuarios[pos_pareja].setEstado(4);
												vector_usuarios[pos].setEstado(4);
												// We obtain the position of the couple in the users array and modify
												if (vector_salas.size() < 10)
												{
													sala new_sala;
													// Player 1 always starts the turn
													vector_usuarios[pos].setTurno(false);
													vector_usuarios[pos_pareja].setTurno(true);
													vector_usuarios[pos].setBandera("B");
													vector_usuarios[pos_pareja].setBandera("A");
													// I insert the players into the room
													new_sala.setJugador1(vector_usuarios[pos]);
													new_sala.setJugador2(vector_usuarios[pos_pareja]);
													new_sala.setCapacidad(2);
													vector_salas.push_back(new_sala);
													mostrarSalas(vector_salas);
													// End of room creation
													int pos_sala = buscar_sala(vector_salas, vector_usuarios[pos].getId());
													bzero(buffer, sizeof(buffer));
													//I send a message to clients
													string salida = "+OK. Player: " + vector_usuarios[pos_pareja].getNombre() + " VS Player: " +
																	vector_usuarios[pos].getNombre() + "\n";

													strcpy(buffer, salida.c_str());
													send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
													send(vector_usuarios[pos_pareja].getId(), buffer, sizeof(buffer), 0);
													salida = "+OK.Board. " + vector_salas[pos_sala].printMatriz(0);
													//I send you the game board
													strcpy(buffer, salida.c_str());
													send(vector_usuarios[pos].getId(), buffer, sizeof(buffer), 0);
													send(vector_usuarios[pos_pareja].getId(), buffer, sizeof(buffer), 0);
												}
												else
												{
													bzero(buffer, sizeof(buffer));
													strcpy(buffer, "-ERR. All the gaming rooms are full.\n");
													send(i, buffer, sizeof(buffer), 0);
												}
											}
											else
											{
												bzero(buffer, sizeof(buffer));
												strcpy(buffer, "+OK. Queued up to find an available opponent.\n");
												send(i, buffer, sizeof(buffer), 0);
											}
										}
									}
								}
								else
								{
									if (buffer_string.find("EXIT") != string::npos)
									{
										salirSala(i, vector_usuarios, vector_salas, 1);
										salirCliente(i, &readfds, vector_usuarios, vector_salas);
									}
									else
									{
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, "-ERR. Invalid command.");
										send(vector_usuarios[posicion].getId(), buffer, sizeof(buffer), 0);
									}
								}
							}
						}
						//If the customer entered ctrl + c
						if (recibidos == 0)
						{
							printf("The client with id -> %d, has exited.\n", i);
							//Delete that socket
							salirSala(i, vector_usuarios, vector_salas, 1);
							salirCliente(i, &readfds, vector_usuarios, vector_salas);
						}
					}
				}
			}
		}
	}
	close(sd);
	return 0;
}
