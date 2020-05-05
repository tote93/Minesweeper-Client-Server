#include "funcionesAuxiliares.hpp"
#include "macros.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <boost/algorithm/string.hpp>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
using namespace std;

int main()
{
	/*----------------------------------------------------
		Socket descriptor and data buffer
	-----------------------------------------------------*/
	int sd;
	struct sockaddr_in sockname;
	char buffer[250];
	socklen_t len_sockname;
	fd_set readfds, auxfds;
	int salida;
	int fin = 0;
	/* --------------------------------------------------
		Socket opens
	---------------------------------------------------*/
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
		perror("No se puede abrir el socket cliente\n");
		exit(1);
	}

	/* ------------------------------------------------------------------
		The fields of the structure are filled with the IP of the
		server and port of the service we request
	-------------------------------------------------------------------*/
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(2000);
	sockname.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* ------------------------------------------------------------------
		The connection to the server is requested
	-------------------------------------------------------------------*/
	len_sockname = sizeof(sockname);
	if (connect(sd, (struct sockaddr *)&sockname, len_sockname) == -1)
	{
		perror("Error de conexión");
		exit(1);
	}
	// We initialize the structures
	FD_ZERO(&auxfds);
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	FD_SET(sd, &readfds);
	/* ------------------------------------------------------------------
		Information is transmitted
	-------------------------------------------------------------------*/
	do
	{
		auxfds = readfds;
		salida = select(sd + 1, &auxfds, NULL, NULL, NULL);
		// I have a message from the server
		if (FD_ISSET(sd, &auxfds))
		{
			bzero(buffer, sizeof(buffer));
			recv(sd, buffer, sizeof(buffer), 0);
			string buffer_string(buffer);
			std::vector<string> v = explode(buffer_string, ".");

			if (v[0].compare("+OK") == 0)
			{
				if (v[1].compare("Board") == 0)
				{
					std::vector<string> aux, aux2, v3;
					system("clear");
					cout << endl;
					cout << BYELLOW << "      A  B  C  D  E  F  G  H  I  J" << endl;
					cout << "   --------------------------------" << RESET << endl;
					aux = explode(v[2], ";");
					for (int i = 0; i < aux.size(); ++i)
					{
						cout << BYELLOW << "[" << i << "] " << RESET;
						aux2 = explode_user(aux[i], ",");
						for (int j = 0; j < aux2.size(); ++j)
						{
							if (aux2[j] == "*" || aux2[j] == " *")
								cout << BRED << " " << aux2[j] << RESET << " ";
							else
								cout << " " << aux2[j] << " ";
						}
						cout << endl;
					}
					cout << BYELLOW << "   --------------------------------" << RESET << endl;
				}
				else
					cout << BBLUE << v[1] << RESET << endl;
			}
			else if (v[0].compare("-ERR") == 0)
			{
				if (v[1].compare(" Server disconnection") == 0)
				{
					cout << BRED << v[1] << RESET << endl;
					fin = 1;
				}
				else
				{
					if (v[1].compare(" Too many connected clients") == 0)
					{

						cout << BRED << v[1] << RESET << endl;
						fin = 1;
					}
					else
						cout << BRED << v[1] << RESET << endl;					
				}
			}
		}
		else
		{
			//I have entered information by keyboard
			if (FD_ISSET(0, &auxfds))
			{
				bzero(buffer, sizeof(buffer));
				fgets(buffer, sizeof(buffer), stdin);
				if (strcmp(buffer, "EXIT\n") == 0)
					fin = 1;
				send(sd, buffer, sizeof(buffer), 0);
			}
		}
	} while (fin == 0);
	close(sd);
	return 0;
}
