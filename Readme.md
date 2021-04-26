# WEndex.taxi v2.0

## Description

WEndex.taxi for Windows only

Consists of:

1. Server side
2. Sample client side
    1. Console app for passengers/drivers
    2. Console app for admin

## Installation

You will need to build the program from source code.

In order to run the program you need to provide a database. Or you can use the sample `app/db.json`

Make sure to follow the name convention and the structure:

```cpp
{
   "speed":0.1,
   "parkRightInFrontOfTheEntrancePrice":300,
   "freeBottleOfWaterPrice":100,
   "prices":[
      {
         "name":"(E)conomy",
         "price":1000
      },
      {
         "name":"(C)omfort",
         "price":1500
      },
      {
         "name":"Comfort(+)",
         "price":1700
      },
      {
         "name":"(B)usiness",
         "price":4000
      }
   ],
   "cities":{
      "Challenge-Brownsville":{
         "latitude":39.46447,
         "longitude":-121.26338
      },
      "Florence-Graham":{
         "latitude":33.96772,
         "longitude":-118.24438
      }
   }
}
```

## How to run

**System**

You should double check the correctness of the format of your database, then initialize the `System`:

```cpp
int main() {
	System system;

	return 0;
}
```

Now the `System` is ready to use.

**Console apps (for both passenger/driver app and admin app)**

You should have your System running to use the console app.

Initialize `GatewayWrapper` class passing the IP of the server to it.

If the Server and the Client are to be used on the same machine you can just pass `localhost`

```cpp
char IP[] = "localhost";
GatewayWrapper gatewayWrapper(IP);
```

The sample Console app uses second argument passed to it as the IP, so you can run it as:

```cpp
app.exe localhost
```

## Requests supported by the System

**signup** - creates a new account

**login** - authorizes a user

**orderRequest** - check the validity of source and destination cities

**createOrder** - creates an order and makes it available to be accepted by a driver

**showOrder** - returns information about the order

**listOrders** - returns IDs of all orders of a user

**findAvailableOrders** - returns IDs of orders suitable for a driver

**acceptOrder** - allows driver to accept an order

**drive** - simulates an event of moving of a car

**coordRequest** - returns coordinates of a car

**updatePaymentMethod** - updates payment methods of a user

**updatePinnedAddresses** - updates pinned addresses of a user

**getCarInfo** - returns information about a car of a driver

**updateStatus** - updates status of a driver

**banUser** - ban user by email

**listCars** - get IDs of all cars in the database

**approveCar** - allow car for rides

**listUserCars** - get IDs of all cars of the specified user

**changeMainCar** - set up the car with the specified ID as the main for a user

**addCar** - specified a car to be owned by a particular user

**listUsers** - get email of all users in the database

## Sample console app demonstration

The following console logs demonstrate the work of two instances of passenger/driver apps.

First is for passenger, second - for driver.

```cpp
------------------------------------------
WEndex.Taxi is glad to see you!
Are you a (P)assenger or (D)river?
P
Successfully connected to server
Do you want to (L)ogin or (S)ign up?
S
Email: test@mail.ru
Password: 1234
What is you full name?
Alice
You need to specify one payment method.
Which one would you specify?
(W)Endex.Money
(Q)uiui
(T)inkon
(S)derbank
W
Specify wallet number: 2048
Type in a command or type /help
/reqorder
Specify your location (or type /quit to cancel the procedure): Eureka
Specify destination: Weed
Route: Eureka-Weed
Estimated time: 21.727043
Prices:
(E)conomy: 2172.704341
(C)omfort: 3259.056512
Comfort(+): 3693.597380
(B)usiness: 8690.817364
E
Please specify payment method:
(W)Endex.Money
(Q)uiui
(T)inkon
(S)derbank
W
Do you want to (U)se your saved wallet or (C)reate new (or replace the old in case you have it)?
U
Successfully created an order! It was assigned an ID 5
Type in a command or type /help
/showorder
Please, specify an ID of the order you want to look up: 5
Source location: Eureka
Destination: Weed
Car type: 0
Passenger name: Alice
Status: Completed
Price: 1000
Payment method: W
Wallet: default
Number of free bottles of water: 0
Option: park right in front of the entrance: false
Type in a command or type /help
```

```cpp
------------------------------------------
WEndex.Taxi is glad to see you!
Are you a (P)assenger or (D)river?
D
Successfully connected to server
Do you want to (L)ogin or (S)ign up?
S
Email: driver@mail.ru
Password: 4567
What is you full name?
Bob
You need to specify characteristics of your car.
Model: Toyota Prius
Color: White
Plate number: AA100AB13
Type in a command or type /help
/mycar
Your car specification:
Car ID: 1
Type assigned by WEndex: 0
Model: Toyota Prius
Licence plate number: AA100AB13
Color: White
Approved: No
Your car is now located on: 35, -120
Type in a command or type /help
/updstatus
Which status do you want to assign to yourself? (I)dle or (W)orking?
W
Successfully updated status to 'Working'
Type in a command or type /help
/availorders
Orders available to you:
5
Type in a command or type /help
/acceptorder
Specify the number of the order: 5
Successfully accepted the order 5
Current location: 35.081245099919585, -120.05830294794482
Current location: 35.16249019983917, -120.11660589588965
Current location: 35.243735299758754, -120.17490884383447
Current location: 35.32498039967834, -120.2332117917793
Current location: 35.40622549959793, -120.29151473972412

**SKIPPED 80 LINES**

Current location: 41.2635277064836, -122.84190724325165
Current location: 41.29648897123243, -122.74749561776963
Current location: 41.32945023598126, -122.6530839922876
Current location: 41.3624115007301, -122.55867236680558
Current location: 41.395372765478925, -122.46426074132356
Current location: 41.42265, -122.38613
No orders in action
Type in a command or type /help
```

## Other information

This is not stable release of a program. Only 3 connections at the same time were tested.

It was successfully compiled using MS VS 2019 and run on Windows 10 x64 2004.

You are running this program at your own risk.

Take care.

Copyright © 2021. All rights reserved.