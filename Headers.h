#pragma once

enum status_type {
    authorized = 0,
    unauthorized = 1,       // created=1
    created = 2,            // updated=2
    updated = 3,            // erased=3
    erased = 4,             // no_need=4
    no_need = 5,            // in_need=5
    in_need = 6,            // unauthorized=6
    service_unavailable = 7,
    wrong_action = 8
};

enum action_type {
    login = 0,
    synchronize = 1,
    create = 2,
    update = 3,
    erase = 4
};

enum class FileStatus {
    created,
    modified,
    erased
};