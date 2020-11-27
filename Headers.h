#pragma once

// Possible responses of the server to a client action
enum status_type {
    authorized = 0,
    unauthorized = 1,
    created = 2,
    updated = 3,
    erased = 4,
    no_need = 5,
    in_need = 6,
    service_unavailable = 7,
    wrong_action = 8
};

// Possible responses of the client to the server status
enum action_type {
    login = 0,
    synchronize = 1,
    create = 2,
    update = 3,
    erase = 4
};

// Possible status of a file or a directory
enum class FileStatus {
    created,
    modified,
    erased
};