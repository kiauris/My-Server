#include "pch.h"
#include <iostream>
#include <uwebsockets/App.h>
#include <atomic>
#include <regex>
#include <map>

using namespace std;

struct UserConnection {
	string name;
	unsigned long userID;
};

int main()
{
	setlocale(LC_ALL, "Russian");

	int port = 8888;
	std::atomic_ulong latestUserID = 0;
	map<unsigned long, string> users;

	uWS::App().get("/hello", [](auto *response, auto *request) {
		response->writeHeader("Content-Security-Policy", "default-src 'self'")->end("asdf");
	});

	uWS::App().ws<UserConnection>("/*", {
		.open = [&users,&latestUserID](auto* ws) {
			// вот это происходит при первом подключении к серверу
			cout << "WebSocket opened" << endl;
			UserConnection* data = (UserConnection*)ws->getUserData();
			data->userID = ++latestUserID;
			data->name = "New User, no name";
			users[data->userID] = data->name;
			cout << "New user connected ID = " << data->userID << endl;
			// пользователь будет получать общую рассылку
			ws->subscribe("BigBrother"); 
			// пользователь будет получать сообщения от др. польз.
			ws->subscribe("user#" + to_string(data->userID));
			

			//ws->publish("user#" + to_string(data->userID),
			//	"Hello, fellow comrade, state your name to the Soviet Union by SetName=\"your name\"");

			cout << "Our server is now " << to_string(users.size()) << " comrades strong\n";
		},
		.message = [&users](auto* ws, string_view message, uWS::OpCode opCode) {
			// вот это происходит при отправке верверу каких-то сообщений
			ws->send(message, opCode);
			UserConnection* data = (UserConnection*)ws->getUserData();

			// Возможные функции через сообщения
			string setName("SetName=");
			string messageTo("Message_to=");
			string listOfUsers("List_of_users");

			// Установить имя для пользователя
			if (message.find(setName) == 0) {
				// проверка валидности имени
				data->name = message.substr(setName.length(), message.length());
				if(data->name.find(",")!=string::npos)
					ws->publish("user#" + to_string(data->userID),"Invalid name, cannot contain commas");
				else if (data->name.size()>255)
					ws->publish("user#" + to_string(data->userID), "Invalid name, too many symbols");
				else {
					// установка имени
					users[data->userID] = data->name;
					cout << "User ID = " << data->userID << " changed name to " << data->name << endl;
					// оповещение польз. о новом товарище
					ws->publish("BigBrother", "Hello comrades, fellow comrade " + data->name +
						", ID = " + to_string(data->userID) + " joined us");
					ws->publish("user#" + to_string(data->userID), "Very well comrade, you are now allowed to talk to other comrades.");
					ws->publish("user#" + to_string(data->userID), "To send a message use Message_to=ID of your fellow comrade, message");
					ws->publish("user#" + to_string(data->userID), "You can also check online comrades by sending List_of_users");
					ws->publish("user#" + to_string(data->userID), "\nGLORY to the Soviet Union!!!!\n");
				}
			}

			//отправить пользователю сообщение
			if (message.find(messageTo) == 0) {
				auto rest = string(message.substr(messageTo.length()));
				int comma_pos = rest.find(",");
				string receiverID = rest.substr(0, comma_pos);
				// проверка на существующий ID у онлайн пользователей
				if (users.find(stoul(receiverID)) != users.end() && (stoul(receiverID)) != data->userID)
				{
					auto text = rest.substr(comma_pos + 1);
					// сообщение только этому пользователю
					ws->publish("user#" + string(receiverID), data->name + ", ID = "
						+ to_string(data->userID) + ": " + text);
				}
				else 
					ws->publish("user#" + to_string(data->userID), "Invalid ID, cannot send message");				
			}

			// список онлайн пользователей
			if (message.find(listOfUsers) == 0) {
				for (auto el : users)
					if (el.first != data->userID)
						ws->publish("user#" + to_string(data->userID), "Commrade: " + el.second
							+ ", ID = " + to_string(el.first));
			}

		},
		.close = [&users](auto* ws,int code,string_view message) {
			// вот это происходит когда пользователь уходит
			UserConnection* data = (UserConnection*)ws->getUserData();
			ws->publish("BigBrother", "Fellow comrade " + data->name +
				", ID = " + to_string(data->userID) + " has left us");
			users[data->userID].erase();
			cout << "WebSocket closed" << endl;
		}
	}).listen(port, [port](auto* listenSocket) {
		if (listenSocket) {
			std::cout << "Listening for connections on port " << port << endl;
		}
		else
			std::cout << "Shoot! We failed to listen and the App fell through, exiting now!" << std::endl;
		}).run();
		
}
/*
	ввод в консоль для теста
	// создание нового соединения
	ws = new WebSocket("ws://localhost:8888/");
	// пользователь будет получать сообщения в консоль
	ws.onmessage = ({data}) => console.log(data);
	// использование функций из .message
	ws.send("SetName=ИМЯ");
	ws.send("Message_to=");

*/