-- Inventory online schema (auto-created by the gamemode too)
CREATE DATABASE IF NOT EXISTS `inventory_test` DEFAULT CHARACTER SET utf8mb4;
USE `inventory_test`;

CREATE TABLE IF NOT EXISTS `player_inventory` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `owner_name` VARCHAR(24) NOT NULL,
    `slot` TINYINT NOT NULL,
    `invItem` VARCHAR(64) NOT NULL,
    `invModel` INT NOT NULL,
    `invQuantity` INT NOT NULL,
    PRIMARY KEY (`id`),
    UNIQUE KEY `owner_slot` (`owner_name`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
