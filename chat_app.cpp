
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <cerrno>
#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MAKE_DIR(p) mkdir((p), 0755)
#endif
#include <memory>

namespace Utils
{
    std::string currentTimestamp()
    {
        std::time_t now = std::time(nullptr);
        std::tm *t = std::localtime(&now);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
        return std::string(buf);
    }

    std::string trim(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }

    std::string hashPassword(const std::string &password)
    {
        unsigned long hash = 5381;
        for (char c : password)
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
        std::ostringstream oss;
        oss << std::hex << hash;
        return oss.str();
    }

    void printLine(char ch = '-', int len = 60)
    {
        std::cout << std::string(len, ch) << "\n";
    }

    void printHeader(const std::string &title)
    {
        printLine('=');
        int pad = (58 - static_cast<int>(title.size())) / 2;
        std::cout << "║" << std::string(pad, ' ') << title
                  << std::string(std::max(0, 58 - pad - (int)title.size()), ' ') << "║\n";
        printLine('=');
    }

    void pause()
    {
        std::cout << "\n[Press ENTER to continue...]";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

class Message
{
private:
    std::string sender_;
    std::string content_;
    std::string timestamp_;
    bool isPrivate_;
    std::string recipient_; // only used for private messages

public:
    Message() = default;

    Message(const std::string &sender,
            const std::string &content,
            bool isPrivate = false,
            const std::string &recipient = "")
        : sender_(sender), content_(content),
          timestamp_(Utils::currentTimestamp()),
          isPrivate_(isPrivate), recipient_(recipient) {}

    std::string serialise() const
    {
        return timestamp_ + "|" + sender_ + "|" + recipient_ + "|" + (isPrivate_ ? "1" : "0") + "|" + content_;
    }

    // Deserialise
    static Message deserialise(const std::string &line)
    {
        std::istringstream ss(line);
        std::string ts, sender, recipient, priv, content;
        std::getline(ss, ts, '|');
        std::getline(ss, sender, '|');
        std::getline(ss, recipient, '|');
        std::getline(ss, priv, '|');
        std::getline(ss, content);
        Message m;
        m.timestamp_ = ts;
        m.sender_ = sender;
        m.recipient_ = recipient;
        m.isPrivate_ = (priv == "1");
        m.content_ = content;
        return m;
    }

    void display(const std::string &viewerUsername = "") const
    {
        if (isPrivate_)
        {
            std::cout << "\033[35m"; // magenta for DMs
            std::cout << "[" << timestamp_ << "] "
                      << "(DM) " << sender_ << " → " << recipient_
                      << ": " << content_ << "\033[0m\n";
        }
        else
        {
            std::cout << "\033[36m[" << timestamp_ << "]\033[0m "
                      << "\033[33m" << sender_ << "\033[0m"
                      << ": " << content_ << "\n";
        }
        (void)viewerUsername;
    }

    const std::string &getSender() const { return sender_; }
    const std::string &getContent() const { return content_; }
    const std::string &getTimestamp() const { return timestamp_; }
    const std::string &getRecipient() const { return recipient_; }
    bool isPrivate() const { return isPrivate_; }
};

class User
{
private:
    std::string username_;
    std::string passwordHash_;
    bool isOnline_;
    std::string lastSeen_;
    std::string joinedAt_;

public:
    User() : isOnline_(false) {}

    User(const std::string &username, const std::string &passwordHash)
        : username_(username), passwordHash_(passwordHash),
          isOnline_(false),
          joinedAt_(Utils::currentTimestamp()),
          lastSeen_(Utils::currentTimestamp()) {}

    bool authenticate(const std::string &password) const
    {
        return passwordHash_ == Utils::hashPassword(password);
    }

    void login()
    {
        isOnline_ = true;
        lastSeen_ = Utils::currentTimestamp();
    }
    void logout()
    {
        isOnline_ = false;
        lastSeen_ = Utils::currentTimestamp();
    }

    // Serialise  →  "username|passwordHash|joinedAt|lastSeen|isOnline"
    std::string serialise() const
    {
        return username_ + "|" + passwordHash_ + "|" + joinedAt_ + "|" + lastSeen_ + "|" + (isOnline_ ? "1" : "0");
    }

    static User deserialise(const std::string &line)
    {
        std::istringstream ss(line);
        std::string username, hash, joined, seen, online;
        std::getline(ss, username, '|');
        std::getline(ss, hash, '|');
        std::getline(ss, joined, '|');
        std::getline(ss, seen, '|');
        std::getline(ss, online);
        User u;
        u.username_ = username;
        u.passwordHash_ = hash;
        u.joinedAt_ = joined;
        u.lastSeen_ = seen;
        u.isOnline_ = (online == "1");
        return u;
    }

    void displayProfile() const
    {
        std::cout << " Username : " << username_ << "\n"
                  << " Joined   : " << joinedAt_ << "\n"
                  << " Last seen: " << lastSeen_ << "\n"
                  << " Status   : " << (isOnline_ ? "\033[32mOnline\033[0m" : "\033[31mOffline\033[0m") << "\n";
    }

    const std::string &getUsername() const { return username_; }
    const std::string &getPasswordHash() const { return passwordHash_; }
    bool isOnline() const { return isOnline_; }
    const std::string &getLastSeen() const { return lastSeen_; }
    const std::string &getJoinedAt() const { return joinedAt_; }
    void setPasswordHash(const std::string &h) { passwordHash_ = h; }
};

class FileHandler
{
private:
    static const std::string DATA_DIR;
    static const std::string USERS_FILE;
    static const std::string LOG_FILE;

    static std::string roomFile(const std::string &room)
    {
        return DATA_DIR + "room_" + room + ".txt";
    }

    static void ensureDir()
    {

        if (MAKE_DIR("chat_data") != 0 && errno != EEXIST)
            throw std::runtime_error("Cannot create data directory 'chat_data'.");
    }

public:
    static void saveUsers(const std::map<std::string, User> &users)
    {
        ensureDir();
        std::ofstream f(USERS_FILE, std::ios::trunc);
        if (!f)
            throw std::runtime_error("Cannot write users file.");
        for (const auto &kv : users)
            f << kv.second.serialise() << "\n";
    }

    static void saveOneUser(const User &user)
    {
        ensureDir();
        auto users = loadUsers();
        users[user.getUsername()] = user;
        saveUsers(users);
    }

    static std::string membersFile(const std::string &room)
    {
        return DATA_DIR + "room_" + room + "_members.txt";
    }

    static void saveMembers(const std::string &room,
                            const std::set<std::string> &members)
    {
        ensureDir();
        std::ofstream f(membersFile(room), std::ios::trunc);
        if (!f)
            throw std::runtime_error("Cannot write members file.");
        for (const auto &m : members)
            f << m << "\n";
    }

    static std::set<std::string> loadMembers(const std::string &room)
    {
        std::set<std::string> members;
        std::ifstream f(membersFile(room));
        if (!f)
            return members;
        std::string line;
        while (std::getline(f, line))
        {
            line = Utils::trim(line);
            if (!line.empty())
                members.insert(line);
        }
        return members;
    }

    static std::map<std::string, User> loadUsers()
    {
        ensureDir();
        std::map<std::string, User> users;
        std::ifstream f(USERS_FILE);
        if (!f)
            return users;
        std::string line;
        while (std::getline(f, line))
        {
            line = Utils::trim(line);
            if (!line.empty())
            {
                User u = User::deserialise(line);
                users[u.getUsername()] = u;
            }
        }
        return users;
    }

    static void appendMessage(const std::string &room, const Message &msg)
    {
        ensureDir();
        std::ofstream f(roomFile(room), std::ios::app);
        if (!f)
            throw std::runtime_error("Cannot write room file.");
        f << msg.serialise() << "\n";
    }

    static std::vector<Message> loadMessages(const std::string &room)
    {
        std::vector<Message> msgs;
        std::ifstream f(roomFile(room));
        if (!f)
            return msgs;
        std::string line;
        while (std::getline(f, line))
        {
            line = Utils::trim(line);
            if (!line.empty())
                msgs.push_back(Message::deserialise(line));
        }
        return msgs;
    }

    static void log(const std::string &entry)
    {
        ensureDir();
        std::ofstream f(LOG_FILE, std::ios::app);
        if (f)
            f << "[" << Utils::currentTimestamp() << "] " << entry << "\n";
    }

    static void saveRooms(const std::set<std::string> &rooms)
    {
        ensureDir();
        std::ofstream f(DATA_DIR + "rooms.txt", std::ios::trunc);
        if (!f)
            throw std::runtime_error("Cannot write rooms file.");
        for (const auto &r : rooms)
            f << r << "\n";
    }

    static std::set<std::string> loadRooms()
    {
        std::set<std::string> rooms;
        std::ifstream f(DATA_DIR + "rooms.txt");
        if (!f)
            return rooms;
        std::string line;
        while (std::getline(f, line))
        {
            line = Utils::trim(line);
            if (!line.empty())
                rooms.insert(line);
        }
        return rooms;
    }
};

const std::string FileHandler::DATA_DIR = "chat_data/";
const std::string FileHandler::USERS_FILE = "chat_data/users.txt";
const std::string FileHandler::LOG_FILE = "chat_data/activity.log";

class ChatRoom
{
private:
    std::string name_;
    std::string createdBy_;
    std::string createdAt_;
    std::vector<Message> messages_;
    std::set<std::string> members_;

public:
    ChatRoom() = default;

    explicit ChatRoom(const std::string &name, const std::string &creator = "system")
        : name_(name), createdBy_(creator), createdAt_(Utils::currentTimestamp())
    {
        messages_ = FileHandler::loadMessages(name_);
        members_ = FileHandler::loadMembers(name_);
    }

    void addMessage(const Message &msg)
    {
        messages_.push_back(msg);
        FileHandler::appendMessage(name_, msg);
    }

    void addMember(const std::string &username)
    {
        members_.insert(username);
    }

    void removeMember(const std::string &username)
    {
        members_.erase(username);
    }

    bool hasMember(const std::string &username) const
    {
        return members_.count(username) > 0;
    }

    void showHistory(const std::string &viewer, int last = 20) const
    {

        std::vector<Message> fresh = FileHandler::loadMessages(name_);
        Utils::printHeader("  Room: " + name_);
        if (fresh.empty())
        {
            std::cout << "  No messages yet. Be the first to say something!\n";
            Utils::printLine();
            return;
        }
        int start = std::max(0, (int)fresh.size() - last);
        std::cout << "  (Showing last " << std::min(last, (int)fresh.size()) << " messages)\n";
        Utils::printLine();
        for (int i = start; i < (int)fresh.size(); ++i)
        {
            const Message &m = fresh[i];
            if (m.isPrivate())
            {
                if (m.getSender() == viewer || m.getRecipient() == viewer)
                    m.display(viewer);
            }
            else
            {
                m.display(viewer);
            }
        }
        Utils::printLine();
    }

    void showMembers() const
    {

        std::set<std::string> fresh = FileHandler::loadMembers(name_);
        std::cout << " Active members in \033[33m" << name_ << "\033[0m:\n";
        if (fresh.empty())
        {
            std::cout << "  (none)\n";
            return;
        }
        for (const auto &m : fresh)
            std::cout << "  • " << m << "\n";
    }

    // Getters
    const std::string &getName() const { return name_; }
    const std::string &getCreatedBy() const { return createdBy_; }
    const std::string &getCreatedAt() const { return createdAt_; }
    size_t memberCount() const { return members_.size(); }
    const std::set<std::string> &getMembers() const { return members_; }
};

class ChatServer
{
private:
    std::map<std::string, User> users_;
    std::map<std::string, ChatRoom> rooms_;
    std::set<std::string> onlineUsers_;

    ChatServer()
    {
        users_ = FileHandler::loadUsers();

        auto roomNames = FileHandler::loadRooms();
        roomNames.insert("General");
        for (const auto &rn : roomNames)
            rooms_.emplace(rn, ChatRoom(rn, "system"));
        FileHandler::saveRooms(roomNames);
        FileHandler::log("Server started.");
    }

public:
    static ChatServer &instance()
    {
        static ChatServer inst;
        return inst;
    }

    // ── Auth ───────────────────────────────────
    bool registerUser(const std::string &username, const std::string &password)
    {
        if (username.empty() || password.empty())
            return false;
        if (users_.count(username))
            return false;
        users_[username] = User(username, Utils::hashPassword(password));
        FileHandler::saveUsers(users_);
        FileHandler::log("New user registered: " + username);
        return true;
    }

    User *loginUser(const std::string &username, const std::string &password)
    {
        auto it = users_.find(username);
        if (it == users_.end())
            return nullptr;
        if (!it->second.authenticate(password))
            return nullptr;
        it->second.login();
        onlineUsers_.insert(username);

        FileHandler::saveOneUser(it->second);
        FileHandler::log(username + " logged in.");
        return &it->second;
    }

    void logoutUser(const std::string &username)
    {
        auto it = users_.find(username);
        if (it != users_.end())
        {
            it->second.logout();
            FileHandler::saveOneUser(it->second);
        }
        onlineUsers_.erase(username);

        for (auto &kv : rooms_)
        {
            if (kv.second.hasMember(username))
            {
                kv.second.removeMember(username);
                FileHandler::saveMembers(kv.first, kv.second.getMembers());
            }
        }
        FileHandler::log(username + " logged out.");
    }

    bool createRoom(const std::string &name, const std::string &creator)
    {
        if (rooms_.count(name))
            return false;
        rooms_.emplace(name, ChatRoom(name, creator));

        std::set<std::string> rnames;
        for (auto &kv : rooms_)
            rnames.insert(kv.first);
        FileHandler::saveRooms(rnames);
        FileHandler::log(creator + " created room: " + name);
        return true;
    }

    bool joinRoom(const std::string &roomName, const std::string &username)
    {
        auto it = rooms_.find(roomName);
        if (it == rooms_.end())
            return false;
        it->second.addMember(username);
        FileHandler::saveMembers(roomName, it->second.getMembers());
        FileHandler::log(username + " joined room: " + roomName);
        return true;
    }

    void leaveRoom(const std::string &roomName, const std::string &username)
    {
        auto it = rooms_.find(roomName);
        if (it != rooms_.end())
        {
            it->second.removeMember(username);
            FileHandler::saveMembers(roomName, it->second.getMembers());
            FileHandler::log(username + " left room: " + roomName);
        }
    }

    bool sendMessage(const std::string &roomName,
                     const std::string &sender,
                     const std::string &content)
    {
        auto it = rooms_.find(roomName);
        if (it == rooms_.end() || content.empty())
            return false;
        Message msg(sender, content);
        it->second.addMessage(msg);
        return true;
    }

    bool sendPrivateMessage(const std::string &roomName,
                            const std::string &sender,
                            const std::string &recipient,
                            const std::string &content)
    {
        if (!users_.count(recipient) || content.empty())
            return false;
        auto it = rooms_.find(roomName);
        if (it == rooms_.end())
            return false;
        Message msg(sender, content, true, recipient);
        it->second.addMessage(msg);
        FileHandler::log(sender + " sent DM to " + recipient);
        return true;
    }

    void showRooms() const
    {
        Utils::printHeader("  Available Rooms");
        for (const auto &kv : rooms_)
        {
            std::cout << "  \033[33m[" << kv.first << "]\033[0m"
                      << "  —  " << kv.second.memberCount() << " active member(s)\n";
        }
        Utils::printLine();
    }

    void showOnlineUsers() const
    {

        auto freshUsers = FileHandler::loadUsers();
        Utils::printHeader("  Online Users");
        bool anyOnline = false;
        for (const auto &kv : freshUsers)
        {
            if (kv.second.isOnline())
            {
                std::cout << "  \033[32m●\033[0m " << kv.first << "\n";
                anyOnline = true;
            }
        }
        if (!anyOnline)
            std::cout << "  No one is online.\n";
        Utils::printLine();
    }

    void showAllUsers() const
    {

        auto freshUsers = FileHandler::loadUsers();
        Utils::printHeader("  All Registered Users");
        for (const auto &kv : freshUsers)
        {
            std::cout << "  " << (kv.second.isOnline() ? "\033[32m●\033[0m " : "\033[31m●\033[0m ")
                      << kv.first
                      << "  (last seen: " << kv.second.getLastSeen() << ")\n";
        }
        Utils::printLine();
    }

    ChatRoom *getRoom(const std::string &name)
    {
        auto it = rooms_.find(name);
        return (it != rooms_.end()) ? &it->second : nullptr;
    }

    bool roomExists(const std::string &name) const { return rooms_.count(name) > 0; }
    bool userExists(const std::string &uname) const { return users_.count(uname) > 0; }
    const std::set<std::string> &getOnlineUsers() const { return onlineUsers_; }
};

class Session
{
private:
    User *user_;
    std::string currentRoom_;
    ChatServer &server_;

    void printRoomPrompt() const
    {
        std::cout << "\n\033[32m[" << user_->getUsername() << "@"
                  << currentRoom_ << "]\033[0m > ";
    }

    void handleRoomCommands()
    {
        std::string input;
        ChatRoom *room = server_.getRoom(currentRoom_);
        if (!room)
        {
            std::cout << " Room not found.\n";
            return;
        }

        room->showHistory(user_->getUsername());
        std::cout << "\n Commands inside room:\n"
                  << "  /msg <user> <text>  — private message\n"
                  << "  /members            — list members\n"
                  << "  /history            — refresh history\n"
                  << "  /leave              — leave room\n"
                  << "  Anything else       — send as message\n";
        Utils::printLine();

        while (true)
        {
            printRoomPrompt();
            std::getline(std::cin, input);
            input = Utils::trim(input);
            if (input.empty())
                continue;

            if (input == "/leave")
            {
                server_.leaveRoom(currentRoom_, user_->getUsername());
                currentRoom_ = "";
                break;
            }
            else if (input == "/members")
            {
                room->showMembers();
            }
            else if (input == "/history")
            {
                room->showHistory(user_->getUsername());
            }
            else if (input.substr(0, 4) == "/msg")
            {

                std::istringstream ss(input.substr(5));
                std::string target;
                ss >> target;
                std::string text;
                std::getline(ss, text);
                text = Utils::trim(text);
                if (target.empty() || text.empty())
                {
                    std::cout << " Usage: /msg <username> <message>\n";
                }
                else if (!server_.userExists(target))
                {
                    std::cout << " User '" << target << "' not found.\n";
                }
                else
                {
                    server_.sendPrivateMessage(currentRoom_, user_->getUsername(), target, text);
                    std::cout << " \033[35m[DM sent to " << target << "]\033[0m\n";
                }
            }
            else
            {
                if (!server_.sendMessage(currentRoom_, user_->getUsername(), input))
                {
                    std::cout << " Failed to send message.\n";
                }
                else
                {

                    room->showHistory(user_->getUsername());
                }
            }
        }
    }

public:
    explicit Session(User *user) : user_(user), server_(ChatServer::instance()) {}

    void run()
    {
        Utils::printHeader("  Welcome, " + user_->getUsername() + "!");
        std::cout << "  Logged in at: " << Utils::currentTimestamp() << "\n";
        Utils::printLine();

        std::string choice;
        while (true)
        {
            std::cout << "\n\033[36m─── Main Menu ───────────────────────────────\033[0m\n"
                      << "  1. Browse & Join Rooms\n"
                      << "  2. Create a New Room\n"
                      << "  3. View My Profile\n"
                      << "  4. Online Users\n"
                      << "  5. All Users\n"
                      << "  6. Logout\n"
                      << "\033[36m─────────────────────────────────────────────\033[0m\n"
                      << " Choice: ";
            std::getline(std::cin, choice);
            choice = Utils::trim(choice);

            if (choice == "1")
            {
                server_.showRooms();
                std::cout << " Enter room name to join: ";
                std::string rname;
                std::getline(std::cin, rname);
                rname = Utils::trim(rname);
                if (!server_.roomExists(rname))
                {
                    std::cout << " Room '" << rname << "' does not exist. Create it first.\n";
                }
                else
                {
                    server_.joinRoom(rname, user_->getUsername());
                    currentRoom_ = rname;
                    handleRoomCommands();
                }
            }
            else if (choice == "2")
            {
                std::cout << " New room name: ";
                std::string rname;
                std::getline(std::cin, rname);
                rname = Utils::trim(rname);
                if (rname.empty())
                {
                    std::cout << " Room name cannot be empty.\n";
                }
                else if (server_.roomExists(rname))
                {
                    std::cout << " Room already exists.\n";
                }
                else
                {
                    server_.createRoom(rname, user_->getUsername());
                    std::cout << " \033[32mRoom '" << rname << "' created!\033[0m\n";
                    server_.joinRoom(rname, user_->getUsername());
                    currentRoom_ = rname;
                    handleRoomCommands();
                }
            }
            else if (choice == "3")
            {
                Utils::printHeader("  Profile");
                user_->displayProfile();
                Utils::printLine();
                std::cout << " Change password? (y/n): ";
                std::string yn;
                std::getline(std::cin, yn);
                if (Utils::trim(yn) == "y")
                {
                    std::cout << " New password: ";
                    std::string np;
                    std::getline(std::cin, np);
                    if (!np.empty())
                    {
                        user_->setPasswordHash(Utils::hashPassword(np));
                        FileHandler::saveUsers(

                            std::map<std::string, User>{});

                        auto users = FileHandler::loadUsers();
                        users[user_->getUsername()].setPasswordHash(Utils::hashPassword(np));
                        FileHandler::saveUsers(users);
                        std::cout << " \033[32mPassword updated.\033[0m\n";
                    }
                }
            }
            else if (choice == "4")
            {
                server_.showOnlineUsers();
            }
            else if (choice == "5")
            {
                server_.showAllUsers();
            }
            else if (choice == "6")
            {
                server_.logoutUser(user_->getUsername());
                std::cout << " Goodbye, " << user_->getUsername() << "!\n";
                break;
            }
            else
            {
                std::cout << " Invalid option.\n";
            }
        }
    }
};

class ChatApplication
{
private:
    ChatServer &server_;

    void showWelcomeBanner() const
    {
        std::cout << "\n";
        Utils::printLine('=');
        std::cout << "║                                                          ║\n"
                  << "║        \033[36m  ██████╗██╗  ██╗ █████╗ ████████╗             \033[0m   ║\n"
                  << "║        \033[36m ██╔════╝██║  ██║██╔══██╗╚══██╔══╝             \033[0m   ║\n"
                  << "║        \033[36m ██║     ███████║███████║   ██║                \033[0m   ║\n"
                  << "║        \033[36m ██║     ██╔══██║██╔══██║   ██║                \033[0m   ║\n"
                  << "║        \033[36m ╚██████╗██║  ██║██║  ██║   ██║                \033[0m   ║\n"
                  << "║        \033[36m  ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝                \033[0m   ║\n"
                  << "║                                                          ║\n"
                  << "║          Terminal Multi-User Chat  —  C++ OOP            ║\n"
                  << "║                                                          ║\n";
        Utils::printLine('=');
        std::cout << "\n";
    }

    User *handleLogin()
    {
        std::string username, password;
        std::cout << " Username: ";
        std::getline(std::cin, username);
        std::cout << " Password: ";
        std::getline(std::cin, password);
        User *u = server_.loginUser(Utils::trim(username), password);
        if (!u)
        {
            std::cout << " \033[31mInvalid credentials.\033[0m\n";
            return nullptr;
        }
        std::cout << " \033[32mLogin successful!\033[0m\n";
        return u;
    }

    bool handleRegister()
    {
        std::string username, password, confirm;
        std::cout << " Choose a username: ";
        std::getline(std::cin, username);
        username = Utils::trim(username);
        if (username.size() < 3 || username.size() > 20)
        {
            std::cout << " Username must be 3–20 characters.\n";
            return false;
        }
        std::cout << " Choose a password: ";
        std::getline(std::cin, password);
        std::cout << " Confirm password : ";
        std::getline(std::cin, confirm);
        if (password != confirm)
        {
            std::cout << " \033[31mPasswords do not match.\033[0m\n";
            return false;
        }
        if (password.size() < 4)
        {
            std::cout << " Password must be at least 4 characters.\n";
            return false;
        }
        if (!server_.registerUser(username, password))
        {
            std::cout << " \033[31mUsername already taken.\033[0m\n";
            return false;
        }
        std::cout << " \033[32mRegistered successfully! Please login.\033[0m\n";
        return true;
    }

public:
    ChatApplication() : server_(ChatServer::instance()) {}

    void run()
    {
        showWelcomeBanner();

        std::string choice;
        while (true)
        {
            std::cout << "\033[36m─── Welcome ─────────────────────────────────\033[0m\n"
                      << "  1. Login\n"
                      << "  2. Register\n"
                      << "  3. Exit\n"
                      << "\033[36m─────────────────────────────────────────────\033[0m\n"
                      << " Choice: ";
            std::getline(std::cin, choice);
            choice = Utils::trim(choice);

            if (choice == "1")
            {
                User *u = handleLogin();
                if (u)
                {
                    Session s(u);
                    s.run();
                    showWelcomeBanner();
                }
            }
            else if (choice == "2")
            {
                handleRegister();
            }
            else if (choice == "3")
            {
                std::cout << " Shutting down. Goodbye!\n";
                FileHandler::log("Server shutdown.");
                break;
            }
            else
            {
                std::cout << " Invalid option.\n";
            }
            std::cout << "\n";
        }
    }
};

int main()
{
    try
    {
        ChatApplication app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "\033[31m[FATAL] " << e.what() << "\033[0m\n";
        return 1;
    }
    return 0;
}
