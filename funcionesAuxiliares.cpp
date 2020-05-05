#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "usuario.hpp"
#include "funcionesAuxiliares.hpp"


void salirSala(int socket, std::vector<usuario> &v, std::vector<sala> &vsala, int opt)
{
	char buffer[250];
	// I get the user's position in the array
	int posicion = buscar_usuario(v, socket);
	// I take the position of the room to be deleted
	int posicion_sala = buscar_sala(vsala, socket);

	int pos_pareja;
	if (posicion_sala != -1)
	{
		// I get the couple from the user with id socket
		usuario pareja = getPareja(vsala, posicion_sala, socket);
		// I get the position of the user's partner with id socket
		pos_pareja = buscar_usuario(v, pareja.getId());
		// I change the status to 2 (waiting to find a partner)
		v[pos_pareja].setEstado(2);
		// I put his turn to true (he will start since he is the one who is waiting)
		v[pos_pareja].setTurno(true);
		// Delete the user from the room
		vsala.erase(vsala.begin() + posicion_sala);
		// If opt == 1 it is because I have not called to clean the room, but that a user has left
		if (opt == 1)
		{
			// I send message to the user partner and later I close socket
			bzero(buffer, sizeof(buffer));
			sprintf(buffer, "+OK. Your partner has left the room, start a game again to play.\n");
			send(v[pos_pareja].getId(), buffer, sizeof(buffer), 0);
		}
	}
}
void salirCliente(int socket, fd_set * readfds, std::vector<usuario> &v, std::vector<sala> &vsala)
{

	char buffer[250];
	int posicion = buscar_usuario(v, socket);
	if (posicion != -1)
	{
		// Delete the user from the vector
		v.erase(v.begin() + posicion);
	}
	close(socket);
	FD_CLR(socket, readfds);
	bzero(buffer, sizeof(buffer));
}


void manejador (int signum)
{
	printf("\nSigint signal received\n");
	signal(SIGINT, manejador);
}
vector<string> explode(const string& str, const string& delim)
{
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length() - 1;
		string token = str.substr(prev, pos - prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	}
	while (pos < str.length() && prev < str.length() - 1);
	return tokens;
}
vector<string> explode_user(const string& str, const string& delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == std::string::npos)
			pos = str.length();
		std::string token = str.substr(prev, pos - prev);
		if (!token.empty())
			tokens.push_back(token);
		prev = pos + delim.length();
	}
	while (pos < str.length() && prev < str.length());
	return tokens;
}

bool existe_usuario(const string &str)
{
	ifstream f("usuarios.txt");
	if (!f.good())
	{
		cout << "The file does not exist\n" << endl;
		return false;
	}
	else
	{
		string nombre, password, a;
		while (f >> nombre >> password)
		{
			if (nombre.compare(str) == 0)
				return true;
		}
		f.close();
		return false;
	}
}
void generar_usuario(const string &str, const string &pass)
{
	ofstream f("usuarios.txt", ios::app);

	if (!f.good())
		cout << "The file does not exist\n" << endl;
	else
	{
		f << str << " " << pass << endl;
		f.close();
	}
}

int buscar_usuario(std::vector<usuario> v, int id, string str )
{
	if (str.compare("") == 0)
	{
		for (int i = 0; i < v.size(); ++i)
		{
			if (v[i].getId() == id)
				return i;
		}
		return -1;
	}
	else
	{

		for (int i = 0; i < v.size(); ++i)
		{
			if (v[i].getNombre().compare(str) == 0)
				return v[i].getEstado();
		}
		return 0;
	}
}

bool comprobar_password(string name, string contrasena)
{
	ifstream f("usuarios.txt");
	if (!f.good())
	{
		cout << "The file does not exist\n" << endl;
		return false;
	}
	else
	{
		string nombre, password;
		while (f >> nombre >> password)
		{
			if (nombre.compare(name) == 0 && password.compare(contrasena) == 0)
				return true;
		}
		f.close();
		return false;
	}
}

usuario buscar_usuario_objeto(std::vector<usuario> v, int id)
{
	usuario user;
	for (int i = 0; i < v.size(); ++i)
	{
		if (v[i].getId() == id)
			return v[i];
	}
	return user;
}
usuario buscar_pareja(std::vector<usuario> v, int id)
{
	usuario user;
	user.setId(-1);
	for (int i = 0; i < v.size(); ++i)
	{
		if (v[i].getEstado() == 3 && v[i].getId() != id)
			return v[i];
	}
	return user;
}

int buscar_sala(std::vector<sala> vsalas, int id)
{

	for (int i = 0; i < vsalas.size(); ++i)
	{
		usuario  user1 = vsalas[i].getJugador1();
		usuario  user2 = vsalas[i].getJugador2();
		if (user1.getId() == id || user2.getId()  == id)
			return i;
	}
	return -1;

}
usuario getPareja(std::vector<sala> vsala, int posicion, int id)
{
	usuario j1 = vsala[posicion].getJugador1();
	usuario j2 = vsala[posicion].getJugador2();
	return (j1.getId() == id) ? vsala[posicion].getJugador2(): vsala[posicion].getJugador1();
}
void mostrarSalas(std::vector<sala> vsala)
{
	usuario user1, user2;
	for (int i = 0; i < vsala.size(); ++i)
	{
		user1 = vsala[i].getJugador1();
		user2 = vsala[i].getJugador2();
		cout << "Room " << i << endl;
		cout << "\t Player 1: " << user1.getNombre() << ", ID: " << user1.getId() << ", Status: " << user1.getEstado() << endl;
		cout << "\t Player 2: " << user2.getNombre() << ", ID: " << user2.getId() << ", Status: " << user2.getEstado() << endl;
		cout << "---------------------------" << endl;
	}
}
int changeToNumber(string value)
{
	char const *salida = value.c_str();
	return *salida - 'A';
}

