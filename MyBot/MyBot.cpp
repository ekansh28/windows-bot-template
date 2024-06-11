
#include "MyBot.h"
#include <dpp/dpp.h>
#include <mysql.h>
#include <iostream>
#include <random>
#include <dpp/dispatcher.h>

const std::string BOT_TOKEN = "MTE0MzgwODQ5NzUxNjA0MDMxMg.GrgCXi.5dnoHWrul_3nGVA_YbFpkQXO39b2d-n7tOHrhc";

MYSQL* connectToDatabase(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname, unsigned int port) {
    MYSQL* conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cerr << "Mysql initialization failed" << std::endl;
        return nullptr;
    }

    std::cout << "CONNECTING TO MYSQL..." << std::endl;

    if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0)) {
        std::cout << "CONNECTED TO MYSQL" << std::endl;
    }
    else {
        std::cerr << "Connection to MySQL failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }
    return conn;
}

bool CheckAndReconnect(MYSQL* conn) {
    // Check if the MySQL connection is still alive
    if (mysql_ping(conn) != 0) {
        // Reconnect to MySQL server
        conn = mysql_real_connect(conn, "system.npowered-hosting.com", "u21_VL6bWAVv9W", "9!GgabT0h2M=zI+4SPKMtAK.", "s21_server",3306, NULL, 0);
        if (conn == NULL) {
            // Failed to reconnect
            std::cerr << "Failed to reconnect to MySQL server: " << mysql_error(conn) << std::endl;
            return false;
        }
    }
    return true;
}

void addServer(MYSQL* conn, dpp::snowflake server_id) {
    // Check if the server_id already exists in SERVER_IDS table
    std::string check_server_id_query = "SELECT 1 FROM SERVER_IDS WHERE server_id = '" + std::to_string(server_id) + "';";
    if (mysql_query(conn, check_server_id_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return; // Exit function on error
    }

    MYSQL_RES* server_id_result = mysql_store_result(conn);
    if (!server_id_result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn) << std::endl;
        return; // Exit function on error
    }

    bool server_id_exists = mysql_num_rows(server_id_result) > 0;
    mysql_free_result(server_id_result);

    // If the server_id does not exist, insert a new row into SERVER_IDS table
    if (!server_id_exists) {
        std::string insert_server_id_query = "INSERT INTO SERVER_IDS (server_id) VALUES ('" + std::to_string(server_id) + "');";
        if (mysql_query(conn, insert_server_id_query.c_str())) {
            std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
            return; // Exit function on error
        }
        std::cout << "Server added to SERVER_IDS table: " << server_id << std::endl;
    }
}


std::string getServerPrefix(MYSQL* conn, dpp::snowflake server_id) {
    std::string prefix_query = "SELECT prefix FROM SERVERINFO WHERE server_id = '" + std::to_string(server_id) + "';";
    if (mysql_query(conn, prefix_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return ""; // Return empty string on error
    }

    MYSQL_RES* prefix_result = mysql_store_result(conn);
    if (!prefix_result) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return ""; // Return empty string on error
    }

    MYSQL_ROW prefix_row = mysql_fetch_row(prefix_result);
    if (!prefix_row) {
        std::cerr << "No prefix found for server ID " << server_id << std::endl;
        mysql_free_result(prefix_result);
        return ""; // Return empty string if no prefix found
    }

    std::string prefix = prefix_row[0]; // Get the prefix from the result
    mysql_free_result(prefix_result);
    return prefix;
}

std::string ChangeServerPrefix(MYSQL* conn, dpp::snowflake server_id, const std::string& new_prefix) {
    // Construct the update query
    std::string update_query = "UPDATE SERVERINFO SET prefix = '" + new_prefix + "' WHERE server_id = '" + std::to_string(server_id) + "';";

    // Execute the update query
    if (mysql_query(conn, update_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return ""; // Return empty string on error
    }

    // Return the new prefix
    return new_prefix;
}

std::string UpdatePoints(MYSQL* conn, dpp::snowflake user_id, dpp::snowflake server_id, int points_to_add) {
    // Ensure the connection is active and reconnect if necessary
    if (!CheckAndReconnect(conn)) {
        return "Failed to reconnect to MySQL server.";
    }

    // Check if the user_id exists in USER_IDS table
    std::string check_user_id_query = "SELECT 1 FROM USER_IDS WHERE user_id = '" + std::to_string(user_id) + "';";
    if (mysql_query(conn, check_user_id_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return "Failed"; // Return "Failed" on error
    }

    MYSQL_RES* user_id_result = mysql_store_result(conn);
    if (!user_id_result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn) << std::endl;
        return "Failed"; // Return "Failed" on error
    }

    bool user_id_exists = mysql_num_rows(user_id_result) > 0;
    mysql_free_result(user_id_result);

    // If the user_id does not exist, insert a new row into USER_IDS table
    if (!user_id_exists) {
        std::string insert_user_id_query = "INSERT INTO USER_IDS (user_id, server_id) VALUES ('" + std::to_string(user_id) + "', '" + std::to_string(server_id) + "');";
        if (mysql_query(conn, insert_user_id_query.c_str())) {
            std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
            return "Failed"; // Return "Failed" on error
        }
    }

    // Check if the user exists in USERINFO table
    std::string check_userinfo_query = "SELECT 1 FROM USERINFO WHERE user_id = '" + std::to_string(user_id) +
        "' AND server_id = '" + std::to_string(server_id) + "';";

    if (mysql_query(conn, check_userinfo_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return "Failed"; // Return "Failed" on error
    }

    MYSQL_RES* userinfo_result = mysql_store_result(conn);
    if (!userinfo_result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn) << std::endl;
        return "Failed"; // Return "Failed" on error
    }

    bool user_exists = mysql_num_rows(userinfo_result) > 0;
    mysql_free_result(userinfo_result);

    // If the user does not exist, insert a new row with points initialized to 0
    if (!user_exists) {
        std::string insert_userinfo_query = "INSERT INTO USERINFO (user_id, server_id, points) VALUES ('" +
            std::to_string(user_id) + "', '" + std::to_string(server_id) + "', 0);";

        if (mysql_query(conn, insert_userinfo_query.c_str())) {
            std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
            return "Failed"; // Return "Failed" on error
        }
    }

    // Construct the query to update points in USERINFO table
    std::string update_query = "UPDATE USERINFO SET points = points + " + std::to_string(points_to_add) +
        " WHERE user_id = '" + std::to_string(user_id) +
        "' AND server_id = '" + std::to_string(server_id) + "';";

    // Execute the update query
    if (mysql_query(conn, update_query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return "Failed"; // Return "Failed" on error
    }

    return "Success"; // Return "Success" if the update was successful
}




std::string GetPoints(MYSQL* conn, dpp::snowflake user_id ,dpp::snowflake server_id) {
    // Construct the query to get points from USERINFO table
    std::string query = "SELECT points FROM USERINFO WHERE user_id = '" + std::to_string(user_id) + "' AND server_id = '" + std::to_string(server_id) + "';";

    // Execute the query
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(conn) << std::endl;
        return ""; // Return empty string on error
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "MySQL store result error: " << mysql_error(conn) << std::endl;
        return ""; // Return empty string on error
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        std::cerr << "No points found for user ID " << user_id << " in server ID " << server_id << std::endl;
        mysql_free_result(result);
        return ""; // Return empty string if no points found
    }

    std::string points = row[0]; // Get the points from the result
    mysql_free_result(result);

    return points; // Return the points as a string
}




int main() {
    std::string hostname = "system.npowered-hosting.com";
    std::string username = "u21_VL6bWAVv9W";
    std::string password = "9!GgabT0h2M=zI+4SPKMtAK.";
    std::string dbname = "s21_server";
    unsigned int port = 3306;
    MYSQL* conn = connectToDatabase(hostname, username, password, dbname, port);






    /* Create bot cluster */
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);

 

    /* Output simple log messages to stdout */
    bot.on_log(dpp::utility::cout_logger());



    


    bot.on_guild_create([&conn](const dpp::guild_create_t& event) {
        addServer(conn, event.created->id);
        });

    // Handle message create event
    bot.on_message_create([&bot, conn](const dpp::message_create_t& event) {

        if (event.msg.author.id == bot.me.id) {
        // Ignore the bot's own messages
            return;
        }
        dpp::snowflake server_id = event.msg.guild_id;
        dpp::snowflake user_id = event.msg.author.id;
        dpp::snowflake channel_id = event.msg.channel_id;

        int points_to_add = 5; 
        std::string update_result = UpdatePoints(conn, user_id, server_id, points_to_add); 
        if (update_result != "Success") {
            std::cerr << "Failed to update points for user " << user_id << " in server " << server_id << std::endl;
        }

        std::string prefix = getServerPrefix(conn, server_id);

        if (prefix.empty()) {
            std::cerr << "Failed to retrieve prefix for server " << server_id << std::endl;
            return;
        }

        if (event.msg.content.starts_with( prefix +"penis")) {
            dpp::user user = event.msg.author;
            std::random_device rd;
            std::uniform_int_distribution<int> dist(1, 20);
            int size = dist(rd);
            std::string size_visual(size, '=');

            event.reply("**" + user.global_name + "**" + "'s penis is **" + std::to_string(size) + "** inches! **8" + size_visual + "D**");
        } 
 
        if (event.msg.content == prefix + "p") {
            dpp::user user = event.msg.author;
            dpp::snowflake server_id = event.msg.guild_id;
            dpp::snowflake user_id = user.id;

            std::string points = GetPoints(conn, user_id, server_id);
            if (!points.empty()) {
                event.reply("You have **" + points + "** points.");
            }
            else {
                event.reply("Failed to retrieve points or you have no points.");
            }
        }
        if (event.msg.content == prefix + "ping") {
            // Convert the double value to string
            std::string ping = std::to_string(bot.rest_ping * 1000);
            event.reply(ping);
        }
        if (event.msg.content == "ping") {
            // Convert the double value to string
            std::string ping = std::to_string(bot.rest_ping * 1000);
            event.reply(ping);
        }
    });










    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });













    bot.on_slashcommand([&bot, conn](const dpp::slashcommand_t& event) {
        dpp::command_interaction cmd_data = event.command.get_command_interaction();

        if (event.command.get_command_name() == "prefix") {
            dpp::snowflake server_id = event.command.guild_id;
            dpp::snowflake user_id = event.command.get_issuing_user().id;

            dpp::permission permission = event.command.get_resolved_permission(user_id);
            bool is_admin = permission.can(dpp::p_administrator);

            if (!is_admin) {
                event.reply("You do not have permission to use this command.");
                return;
            }

            auto subcommand = cmd_data.options[0];

            if (subcommand.name == "set") {
                event.thinking(false);
                    // Check if the subcommand has any options.
                    if (!subcommand.options.empty()) {
                        // Get the prefix from the parameter
                        std::string new_prefix = std::get<std::string>(subcommand.options[0].value);

                        std::string result = ChangeServerPrefix(conn, server_id, new_prefix);

                        if (!result.empty()) {
                            event.edit_original_response(dpp::message("Prefix set to: " + new_prefix));
                        }
                        else {
                            event.edit_original_response(dpp::message("Failed to set prefix."));
                        }
                    }
                    else {
                        // Reply if there were no options.
                        event.edit_original_response(dpp::message("No prefix specified."));
                    }
                
            }
        }
        });


    /* Register slash command here in on_ready */
    bot.on_ready([&bot](const dpp::ready_t& event) {
        /* Wrap command registration in run_once to make sure it doesnt run on every full reconnection */
        if (dpp::run_once<struct register_bot_commands>()) {
            std::vector<dpp::slashcommand> commands{
                { "ping", "Ping pong!", bot.me.id }
            };
            bot.global_bulk_command_create(commands);


            dpp::slashcommand prefix("prefix", "set prefix", bot.me.id);

            prefix.add_option(
                /* Create a subcommand type option for "set". */
                dpp::command_option(dpp::co_sub_command, "set", "set a prefix")
                .add_option(dpp::command_option(dpp::co_string, "prefix", "New prefix", true))
            );
           
            /* Create command */
            bot.global_command_create(prefix);
        }
    });

    /* Start the bot */
    bot.start(dpp::st_wait);


    std::cout << "Ending program..." << std::endl;
    return 0;
}
