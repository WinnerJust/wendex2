#include <iostream>
#include <string>
#include <utility>
#include <ctime>
#include <cstdlib>
#include <thread>
#include "json.hpp"
#include "gateways.h"
#include "safeInput.h"

//For convenience
using json = nlohmann::json;


GatewayWrapper::GatewayWrapper(char** argv) {
	srand(time(0));

	while (1) {
		std::cout << "------------------------------------------\n";
		std::cout << "WEndex.Taxi Admin panel \n";
		auto admin = AdminGateway(argv);
	}
}

AdminGateway::AdminGateway(char** argv) : client(argv) {
	json loginData;

	std::cout << "Do you want to (L)ogin or (S)ign up?\n";
	char c = inputChar("LlSs");

	std::cout << "Email: ";
	std::string s = inputString(true);

	loginData["email"] = s;
	email = s;

	std::cout << "Password: ";
	s = inputString(true);

	loginData["password"] = s;
	userData["password"] = s;

	//Performing specified action - login or signup
	if (c == 'L' || c == 'l') {
		json resp = client.request({ {"dataType", "login"}, {"data", loginData} });

		if (resp["dataType"] == "error") {
			std::cout << resp["message"].get<std::string>() << '\n';
			return;
		}
		else {
			userData = resp["data"];
		}
	} else {
		userData["userType"] = "A";
		userData["rating"] = (double)(rand() % 50) / 10;

		std::cout << "What is you full name?\n";
		userData["name"] = inputString(false);

		json resp = client.request({ { "dataType", "signup" }, { "data", {{"email", email}, {"data", userData}} } });

		if (resp["dataType"] == "error") {
			std::cout << resp["message"].get<std::string>() << '\n';
			return;
		}
		else {
			userData = resp["data"];
		}
	}

	//Starts infinite event catcher for passenger
	while (1) {
		eventCatcher();
	}
}



void AdminGateway::eventCatcher() {
	std::cout << "Type in a command or type /help\n";

	std::string s = inputString(true);

	if (s == "/help") {
		std::cout << "/showorder - Get information on the order by ID\n";
		std::cout << "/carloc - Get location of the car on active order\n";
		std::cout << "/showbill - Get the bill of the order by ID\n";
		std::cout << "/listusers - Get list of emails of all users\n";
		std::cout << "/banuser - Ban user for specified amount of time\n";
		std::cout << "/listcars - Get list of IDs of all cars\n";
		std::cout << "/approvecar - Approve a car by ID\n";
	}
	else if (s == "/showorder") showOrder();
	else if (s == "/carloc") carLoc();
	else if (s == "/showbill") showBill();
	else if (s == "/listusers") listUsers();
	else if (s == "/banuser") banUser();
	else if (s == "/listcars") listCars();
	else if (s == "/approvecar") approveCar();
}



void AdminGateway::listUsers() {
	//Server requests every query to have data key, so we send empty data
	json resp = client.request({ {"dataType", "listUsers"}, {"data", {{}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << "Emails of users registered in the system: \n";
		for (auto& i : resp["data"]) {
			std::cout << i.get<std::string>() << '\n';
		}
	}
}


void AdminGateway::approveCar() {
	std::cout << "Please, specify an ID of the car you want to approve: ";
	int res = inputNumber();

	json resp = client.request({ {"dataType", "approveCar"}, {"data", {{"carID", res}}} });

	std::cout << resp["message"].get<std::string>() << "\n";
}

void AdminGateway::listCars() {
	json resp = client.request({ {"dataType", "listCars"}, {"data", {{}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << "ID of cars registered in the system: \n";
		for (auto& i : resp["data"]) {
			std::cout << i.get<int>() << '\n';
		}
	}
}

void AdminGateway::banUser() {
	std::cout << "Please, specify an email of the user you want to ban: ";
	std::string res = inputString(true);

	std::cout << "Please, specify a duration of a ban in days: ";
	int duration = inputNumber();


	json resp = client.request({ {"dataType", "banUser"}, {"data", {{"email", res}, {"duration", duration}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
}


void AdminGateway::carLoc() {
	json resp = client.request({ {"dataType", "coordRequest"}, {"data", {{"email", email}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << "Car that is on your active order is now on " << resp["data"]["carPos"]["latitude"]
			<< ", " << resp["data"]["carPos"]["longitude"] << '\n';
	}
}


//This method print the information of the order related to payment (price, payment method and wallet number)
void AdminGateway::showBill() {
	std::cout << "Please, specify an ID of the order you want to get the bill of: ";
	int res = inputNumber();


	json resp = client.request({ {"dataType", "showOrder"}, {"data", {{"orderID", res}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << "Price: " << resp["data"]["price"] << '\n';
		std::cout << "Payment method: " << resp["data"]["paymentMethod"].get<std::string>() << '\n';
		std::cout << "Wallet: " << resp["data"]["paymentValue"].get<std::string>() << '\n';
	}
}


void AdminGateway::showOrder() {
	std::cout << "Please, specify an ID of the order you want to look up: ";
	int res = inputNumber();


	json resp = client.request({ {"dataType", "showOrder"}, {"data", {{"orderID", res}}} });

	if (resp["dataType"] == "error") {
		std::cout << resp["message"].get<std::string>() << "\n";
	}
	else {
		std::cout << "Source location: " << resp["data"]["from"].get<std::string>() << '\n';
		std::cout << "Destination: " << resp["data"]["to"].get<std::string>() << '\n';
		std::cout << "Car type: " << resp["data"]["carType"].get<int>() << '\n';
		std::cout << "Passenger name: " << resp["data"]["passengerName"].get<std::string>() << '\n';
		std::cout << "Status: " << resp["data"]["status"].get<std::string>() << '\n';
		std::cout << "Price: " << resp["data"]["price"].get<double>() << '\n';
		std::cout << "Payment method: " << resp["data"]["paymentMethod"].get<std::string>() << '\n';
		std::cout << "Wallet: " << resp["data"]["paymentValue"].get<std::string>() << '\n';
		std::cout << "Number of free bottles of water: " << resp["data"]["freeBottleOfWater"].get<int>() << '\n';
		std::cout << "Option: park right in front of the entrance: " << resp["data"]["parkRightInFrontOfTheEntrance"] << '\n';
	}
}

