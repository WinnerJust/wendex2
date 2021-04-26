#include "system.h"
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include "algs.h"
#include "errorCodes.h"

//This define generates code that checks the presence of key 'x' in the json object 'data'
//Otherwise returns error that is unique for every key
#define assertKey(x) \
if (!data.contains(#x) || data[#x].is_null()) { \
	return { {"dataType", "error"}, {"message", "Unexpected error " + null_##x + '.' } }; \
}

const std::string dataBasePath = "db.json";



System::System() {
	//Server already got initialized by default constructor
	//Thus, it should be already running

	//Loading database
	std::ifstream fin(dataBasePath);
	fin >> db;

	fin.close();

	//Starting infinite loop based saving thread
	std::thread(&System::dumpDB, this).detach();


	//All of the following code defines event action
	//Meaning, what does the server should do on incoming data

	//I didn't separate the code to different methods because the code is never used twice
	server.setOnData([this](json data) -> json {
		//Check if we even got some data
		if (data.is_null()) {
			return { {"dataType", "error"}, {"message", "Unexpected error " + null_request + '.' } };
		}

		assertKey(dataType);
		assertKey(data);

		//Server logs
		std::cout << data.dump();

		std::string dataType = data["dataType"];
		data = data["data"];

		if (dataType == "signup") {
			assertKey(email);

			std::string email = data["email"];

			if (db["users"].contains(email)) {
				//Returning straight away so that no confusion will occur
				return { {"dataType", "error"}, {"message", "This account already exists." } };
			}
			else {
				//This means if the user is a driver
				if (data.contains("car")) {
					//Data about the car passed along with data specified by user during the sign up

					data["car"]["approved"] = false;
					data["car"]["carID"] = db["cars"].size();

					//We store it in database (in JSON array)
					db["cars"].push_back(data["car"]);
					
					//carID is an index of that array
					data["data"]["carID"] = db["cars"].size() - 1;

					//Adding this car to the array of cars that driver currently has
					data["data"]["carIDs"] = { db["cars"].size() - 1 };

					//Status of a driver (can be Idle or Working)
					data["data"]["status"] = "Idle";
				}
				db["users"][email] = data["data"];

				return { {"dataType", "success"}, {"data", db["users"][email]} };
			}
		}
		else if (dataType == "login") {
			assertKey(email);
			assertKey(password);

			std::string email = data["email"];

			if (!db["users"].contains(email)) {
				return { {"dataType", "error"}, {"message", "Account with this email does not exist." } };
			} 
			//Password check
			//We can check direcly JSON objects
			else if (db["users"][email]["password"] != data["password"]) {
				return { {"dataType", "error"}, {"message", "Wrong password." } };
			}
			else {
				json dataBack = db["users"][email];
				return { {"dataType", "success"}, {"data", dataBack} };
			}
		}
		else if (dataType == "orderRequest") {
			assertKey(from);
			assertKey(to);

			//If specified cities are not in the database
			//We return the city that has the least levenstein distance to specified one
			if (!db["cities"].contains(data["from"])) {
				return { {"dataType", "error"}, 
					{"message", "Specified location city is not found. Maybe you meant "  + findClosestCityName(data["from"]) + '\n'} };
			} else if (!db["cities"].contains(data["to"])) {
				return { {"dataType", "error"},
					{"message", "Specified destination city is not found. Maybe you meant " + findClosestCityName(data["to"]) + '\n'} };
			}
			else {
				return { {"dataType", "success"}, {"message", generateOrder(data["from"], data["to"])},
					{"data", {{"parkRightInFrontOfTheEntrancePrice", db["parkRightInFrontOfTheEntrancePrice"]}, 
					{"freeBottleOfWaterPrice", db["freeBottleOfWaterPrice"]}
				}} };
			}
		}
		else if (dataType == "createOrder") {
			assertKey(from);
			assertKey(to);
			assertKey(email);
			assertKey(carType);
			assertKey(paymentMethod);
			assertKey(paymentValue);

			std::string email = data["email"];

			//Ban check
			if (db["users"][email].contains("banTime") && db["users"][email]["banTime"].get<int>() > time(0)) {
				return { {"dataType", "error"},
					{"message", "You are banned for "
					+ std::to_string(db["users"][email]["banTime"].get<int>() - time(0)) + " more seconds." } };
			}


			json& payments = db["users"][email]["payments"];

			//Checking the existence of a wallet for specified payment method
			if (data["paymentValue"] == "default" &&
				(!payments.contains(data["paymentMethod"]) || payments[data["paymentMethod"].get<std::string>()].is_null())) {
				return { {"dataType", "error"}, 
					{"message", "You do not have default wallet for " + data["paymentValue"].get<std::string>()} };
			}

			//Saving payment method in database if it was not specified as default
			if (data["paymentValue"] != "default") {
				payments[data["paymentMethod"].get<std::string>()] = data["paymentValue"];
			}


			data["passengerName"] = db["users"][email]["name"];

			int orderID = db["orders"].size();
			db["orders"].push_back(data);

			//Calculating the price of a ride
			int price = db["prices"][data["carType"].get<int>()]["price"];
			if (data.contains("freeBottleOfWater") && data["freeBottleOfWater"].is_number_integer()) {
				price += data["freeBottleOfWater"].get<int>() * db["freeBottleOfWaterPrice"].get<int>();
			}
			if (data.contains("parkRightInFrontOfTheEntrance") && data["parkRightInFrontOfTheEntrance"].is_boolean() 
				&& data["parkRightInFrontOfTheEntrance"]) {
				price += db["parkRightInFrontOfTheEntrancePrice"].get<int>();
			}

			db["orders"][orderID]["price"] = price;
			db["orders"][orderID]["status"] = "Searching for driver";

			db["users"][email]["orders"].push_back(orderID);

			return { {"dataType", "success"}, {"data", orderID} };
		}
		else if (dataType == "showOrder") {
			assertKey(orderID);

			if (db["orders"].size() > data["orderID"]) {
				return { {"dataType", "success"}, {"data", db["orders"][data["orderID"].get<int>()] } };
			}
			else {
				return { {"dataType", "error"}, {"message", "This order doesn't exist"} };
			}
		}
		else if (dataType == "listOrders") {
			assertKey(email);

			json res;

			//Adding to res all orders of a user
			for (auto& i : db["users"][data["email"].get<std::string>()]["orders"].items()) {
				res.push_back(i.key());
			}

			return { {"dataType", "success"}, {"data", res} };
		}
		else if (dataType == "findAvailableOrders") {
			assertKey(email);

			std::string email = data["email"];

			//Ban check
			if (db["users"][email].contains("banTime") && db["users"][email]["banTime"].get<int>() > time(0)) {
				return { {"dataType", "error"},
					{"message", "You are banned for "
					+ std::to_string(db["users"][email]["banTime"].get<int>() - time(0)) + " more seconds." } };
			}

			//Status check
			if (db["users"][email]["status"] != "Working") {
				return { {"dataType", "error"}, {"message", "Your status is not 'Working'"} };
			}

			json res;
			
			//Adding to res all orders that need a driver and those that have carType the same as that of the driver's car
			for (int i = 0; i < db["orders"].size(); i++) {
				int carID = db["users"][email]["carID"];
				if (db["orders"][i]["status"] == "Searching for driver" && db["orders"][i]["carType"] == db["cars"][carID]["carType"]) {
					res.push_back(i);
				}
			}

			if (res.size() == 0) {
				return { {"dataType", "error"}, {"message", "No orders are available for you."} };
			}

			return { {"dataType", "success"}, {"data", res} };
		}
		else if (dataType == "acceptOrder") {
			assertKey(email);
			assertKey(orderID);

			std::string email = data["email"];

			//Ban check
			if (db["users"][email].contains("banTime") && db["users"][email]["banTime"].get<int>() > time(0)) {
				return { {"dataType", "error"},
					{"message", "You are banned for "
					+ std::to_string(db["users"][email]["banTime"].get<int>() - time(0)) + " more seconds." } };
			}

			//Status check
			if (db["users"][email]["status"] != "Working") {
				return { {"dataType", "error"}, {"message", "Your status is not 'Working'"} };
			}

			//Car approval check
			int carID = db["users"][email]["carID"];
			if (!db["cars"][carID]["approved"]) {
				return { {"dataType", "error"}, {"message", "The car you are currently using is not approved."} };
			}

			//Changing statuses of the driver and the order
			if (db["orders"].size() > data["orderID"]) {
				db["orders"][data["orderID"].get<int>()]["status"] = "Waiting for driver to come";
				db["orders"][data["orderID"].get<int>()]["driverEmail"] = email;

				db["users"][email]["status"] = "Busy";

				db["users"][email]["orders"].push_back(data["orderID"]);

				return { {"dataType", "success"} };
			}
			else {
				return { {"dataType", "error"}, {"message", "This order doesn't exist"} };
			}
		}
		else if (dataType == "drive") {
			//This order will simulate event of moving of the driver's car
			//It moves the car closer to where it should be to complete the order
			//Either to passenger's location or the destination

			assertKey(email);

			std::string email = data["email"];

			//Finding driver's orders that one that is in action
			int orderID = -1;
			std::string thisStatus = "";
			for (auto& i : db["users"][email]["orders"]) {
				thisStatus = db["orders"][i.get<int>()]["status"];
				if (thisStatus == "Waiting for driver to come" || thisStatus == "Driving to destination") {
					orderID = i;
					break;
				}
			}

			//This is normal behavior (happening at the end of the ride)
			if (orderID == -1) {
				return { {"dataType", "error"}, {"message", "No orders in action" } };
			}

			//This should not happen during normal behavior of program
			//So also unexpected error
			if (db["users"][email]["status"] != "Busy") {
				return { {"dataType", "error"}, {"message", "Unexpected error " + illegal_drive_request + '.' } };
			}

			std::string from = db["orders"][orderID]["from"];
			std::string to = db["orders"][orderID]["to"];

			json& fromCity = db["cities"][from];
			json& toCity = db["cities"][to];

			int carID = db["users"][email]["carID"];
			json& carPos = db["cars"][carID]["position"];

			//We have two states of an order:
			//"Waiting for driver to come" and "Driving to destination"
			if (thisStatus == "Waiting for driver to come") {
				double dist = sqrt(pow(fromCity["latitude"].get<double>() - carPos["latitude"].get<double>(), 2)
					+ pow(fromCity["longitude"].get<double>() - carPos["longitude"].get<double>(), 2));

				double speed = db["speed"].get<double>();

				if (speed >= dist) {
					//If we arrived - changing status
					db["orders"][orderID]["status"] = "Driving to destination";
					carPos["latitude"] = fromCity["latitude"];
					carPos["longitude"] = fromCity["longitude"];
				}
				else {
					//Applying something similar to sin and cos
					carPos["latitude"] = carPos["latitude"].get<double>() +
						speed * (fromCity["latitude"].get<double>() - carPos["latitude"].get<double>()) / dist;
					carPos["longitude"] = carPos["longitude"].get<double>() +
						speed * (fromCity["longitude"].get<double>() - carPos["longitude"].get<double>()) / dist;
				}
			}
			else if (thisStatus == "Driving to destination") {
				double dist = sqrt(pow(toCity["latitude"].get<double>() - carPos["latitude"].get<double>(), 2)
					+ pow(toCity["longitude"].get<double>() - carPos["longitude"].get<double>(), 2));

				double speed = db["speed"].get<double>();

				if (speed >= dist) {
					//If we arrived - changing status
					db["orders"][orderID]["status"] = "Completed";
					db["users"][email]["status"] = "Working";

					carPos["latitude"] = toCity["latitude"];
					carPos["longitude"] = toCity["longitude"];
				}
				else {
					//Applying something similar to sin and cos
					carPos["latitude"] = carPos["latitude"].get<double>() +
						speed * (toCity["latitude"].get<double>() - carPos["latitude"].get<double>()) / dist;
					carPos["longitude"] = carPos["longitude"].get<double>() +
						speed * (toCity["longitude"].get<double>() - carPos["longitude"].get<double>()) / dist;
				}
			}

			return { {"dataType", "success"}, {"data", {{"position", carPos}}} };
		}
		else if (dataType == "coordRequest") {
			assertKey(email);

			//Finding the order that is in action now
			//It should be exactly one
			int orderID = -1;
			std::string thisStatus = "";
			for (auto& i : db["users"][data["email"].get<std::string>()]["orders"]) {
				thisStatus = db["orders"][i.get<int>()]["status"];
				if (thisStatus == "Waiting for driver to come" || thisStatus == "Driving to destination") {
					orderID = i;
					break;
				}
			}

			//This is normal behavior
			if (orderID == -1) {
				return { {"dataType", "error"}, {"message", "No orders in action." } };
			}

			//Finding the car firstly getting the ID of it through the driver
			std::string driver = db["orders"][orderID]["driverEmail"];
			int carID = db["users"][driver]["carID"];
			json carPos = db["cars"][carID]["position"];

			return { {"dataType", "success"}, {"data", {{"position", carPos}}} };
		}
		else if (dataType == "updatePaymentMethod") {
			assertKey(email);
			assertKey(paymentMethod);
			assertKey(paymentValue);

			db["users"][data["email"].get<std::string>()]["payments"][data["paymentMethod"].get<std::string>()] = data["paymentValue"];

			return { {"dataType", "success"} };
		}
		else if (dataType == "updatePinnedAddresses") {
			assertKey(email);
			assertKey(addressID);
			assertKey(addressValue);

			std::string email = data["email"];

			//Checking if the address is correct
			if (!db["cities"].contains(data["addressValue"])) {
				return { {"dataType", "error"},
					{"message", "Specified city is not found. Maybe you meant " + findClosestCityName(data["addressValue"])} };
			}

			//If the number is exactly 1 more than the number of pinned addresses has the current user
			//That we add a new pinned address
			if (db["users"][email]["addresses"].size() == data["addressID"]) {
				db["users"][email]["addresses"].push_back(data["addressValue"]);
				return { {"dataType", "success"} };
			}
			else if (db["users"][email]["addresses"].size() > data["addressID"]) {
				db["users"][email]["addresses"][data["addressID"].get<int>()] = data["addressValue"];
				return { {"dataType", "success"} };
			}
			else {
				return { {"dataType", "error"},
					{"message", "No such address found"} };
			}
		}
		else if (dataType == "getCarInfo") {
			assertKey(email);

			int carID = db["users"][data["email"].get<std::string>()]["carID"];
			json& carData = db["cars"][carID];

			return { {"dataType", "success"}, {"data", {{"carData", carData}}} };
		}
		else if (dataType == "updateStatus") {
			assertKey(email);
			assertKey(status);

			std::string email = data["email"];

			if (data["status"] == "I" || data["status"] == "i")
				data["status"] = "Idle";
			if (data["status"] == "W" || data["status"] == "w")
				data["status"] = "Working";

			//Changing the status if it is needed
			if (db["users"][email]["status"] == data["status"]) {
				return { {"dataType", "error"}, {"message", "You already have status \'" + data["status"] + '\'' } };
			}

			db["users"][email]["status"] = data["status"];

			return { {"dataType", "success"}, {"data", data["status"] } };
		}
		else if (dataType == "banUser") {
			assertKey(email);
			assertKey(duration);

			std::string email = data["email"];

			//Checking if the user exist
			//And applying ban if he is not an admin
			if (db["users"].contains(email) && !db["users"][email].is_null()) {
				if (db["users"][email]["userType"] == "A") {
					return { {"dataType", "error"}, {"message", "You cannot ban an admin." } };
				}

				db["users"][email]["banTime"] = time(0) + data["duration"].get<int>() * 86400;

				return { {"dataType", "success"}, {"message", "Successfully banned the user." }, {"data", data["status"] } };
			}
			else {
				return { {"dataType", "error"}, {"message", "This user doesn't exist." } };
			}
		}
		else if (dataType == "listCars") {
			json carIDs;

			for (auto i : db["cars"].items()) {
				//Value returned by key() is a string by default, so doing the following
				carIDs.push_back(stoi(i.key()));
			}

			return { {"dataType", "success"}, {"data", carIDs } };
		}
		else if (dataType == "approveCar") {
			assertKey(carID);

			int carID = data["carID"].get<int>();

			if (db["cars"].size() <= carID) {
				return { {"dataType", "error"}, {"message", "This car doesn't exist." } };
			}


			json& thisCar = db["cars"][carID];

			if (!thisCar.contains("approved") || !thisCar["approved"]) {
				thisCar["approved"] = true;

				return { {"dataType", "success"}, {"message", "You approved the car."} };
			}
			else {
				return { {"dataType", "error"}, {"message", "Car is already approved." } };
			}
		}
		else if (dataType == "listUserCars") {
			assertKey(email);

			json carIDs;

			for (auto i : db["users"][data["email"].get<std::string>()]["carIDs"]) {
				carIDs.push_back(i);
			}

			return { {"dataType", "success"}, {"data", carIDs } };
		}
		else if (dataType == "changeMainCar") {
			assertKey(email);
			assertKey(carID);

			std::string email = data["email"].get<std::string>();
			int carID = data["carID"].get<int>();

			//Checking if the user owns the car with specified ID
			bool found = false;
			for (auto i : db["users"][email]["carIDs"]) {
				if (i.get<int>() == carID) {
					found = true;
					break;
				}
			}

			if (!found) {
				return { {"dataType", "error"}, {"message", "You do not own this car." } };
			}

			db["users"][email]["carID"] = carID;
			return { {"dataType", "success"}, {"message", "This car is now your main." } };
		}
		else if (dataType == "addCar") {
			assertKey(email);
			assertKey(car);

			data["car"]["approved"] = false;
			data["car"]["carID"] = db["cars"].size();

			db["cars"].push_back(data["car"]);

			//Adding this car to the array of cars that driver currently has
			db["users"][data["email"].get<std::string>()]["carIDs"].push_back(db["cars"].size() - 1);

			return { {"dataType", "success"}, {"message", "You added new car." } };
		}
		else if (dataType == "listUsers") {
			json userEmails;

			for (auto i : db["users"].items()) {
				userEmails.push_back(i.key());
			}

			return { {"dataType", "success"}, {"data", userEmails } };
		}
		else {
			return { {"dataType", "error"}, {"message", "Unexpected error " + unknown_dataType + '.' } };
		}
	});


	std::this_thread::sleep_for(std::chrono::milliseconds(10000000));
}



std::string System::findClosestCityName(std::string name) {
	int minValue = INT_MAX;
	std::string res = "";
	for (auto& i : db["cities"].items()) {
		int dist = levenstein(name, i.key());
		if (dist < minValue) {
			minValue = dist;
			res = i.key();
		}
	}
	return res;
}


std::string System::generateOrder(std::string from, std::string to) {
	std::string res = "Route: " + from + '-' + to + '\n';
	double dist = sqrt(pow(db["cities"][from]["latitude"].get<double>() - db["cities"][to]["latitude"], 2) 
		+ pow(db["cities"][from]["longitude"].get<double>() - db["cities"][to]["longitude"], 2));
	res += "Estimated time: " + std::to_string(dist / db["speed"].get<double>()) + '\n';
	res += "Prices: \n";

	for (auto& i : db["prices"]) {
		res += i["name"].get<std::string>() + ": " + std::to_string(i["price"].get<int>() * dist) + '\n';
	}

	return res;
}


void System::dumpDB() {
	std::this_thread::sleep_for(std::chrono::seconds(60));

	std::ofstream fout(dataBasePath);
	fout << db;

	//Do not forgot to close the file to avoid interrupted writes to database
	fout.close();

	this->dumpDB();
}

