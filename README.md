# Group Chat Applicaiton in C

By:   Randall Kong, Roy Xavier Pimentel

Role: Team 3 - Server

Extended Version: https://github.com/XavierPim/Multi-Client_Chat_Server_IPv4_6

# Setup Instructions:
1) ./generate-flags.sh
2) ./generate-cmakelists.sh
3) ./change-compiler.sh -c [gcc or clang]
4) ./build.sh


# Run
1) ./server [ip address] [port]
2) ./client [ip address] [port]

# Tips
- don't push files .sh executables generate.

# Testing
- connection with client                   {Status - GOOD}
- reliable read/write                      {Status - GOOD}
- works with gcc and clang                 {Status - GOOD}
- multiple concurrent connections          {Status - GOOD}
- commands (/h, /ul, /u, /w)               {Status - GOOD}
- work with all os (MacOS, Manjaro, ??)    {Status - GOOD}

- with client team 6 (Jiang Peng, Jianhua) {Status - GOOD}
- with client team 7 (Dong-il, Tushar)     {Status - GOOD}
- with client team 8 (Kiefer, Jack)        {Status - GOOD}

