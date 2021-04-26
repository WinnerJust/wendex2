#pragma once

#include "client.h"

class GatewayWrapper {
public:
	GatewayWrapper(char** argv);
};

class AdminGateway {
protected:
	Client client;
	json userData;
	std::string email;

	void eventCatcher();

	void showOrder();
	void carLoc();
	void showBill();
	void listUsers();
	void banUser();
	void listCars();
	void approveCar();

public:
	AdminGateway(char** argv);
};