# Terminal Multi-User Chat App (C++)

A terminal-based multi-user chat application built with C++ OOP and file handling.

## Features
- User registration & login (hashed passwords)
- Multiple chat rooms
- Private messaging (`/msg <user> <text>`)
- Message history persisted to disk
- Activity logging

## Compile & Run
g++ -std=c++17 -o chat_app chat_app.cpp
./chat_app

## Usage
- Open two terminals, compile once
- Register/login as different users in each terminal
- Join the same room
- Type /history to see messages from the other terminal