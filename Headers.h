#pragma once

enum status_type {
    ok = 0,
    created = 1,
    updated = 2,
    erased = 3,
    no_need = 4,
    in_need = 5,
    unauthorized = 6,
    service_unavailable = 7
};

enum action_type {
    login = 0,
    synchronize = 1,
    create = 2,
    update = 3,
    erase = 4
};