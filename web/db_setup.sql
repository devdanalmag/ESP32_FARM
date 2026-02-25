-- ==========================================
--  ESP32 FARM DATA COLLECTION SYSTEM
--  MySQL Database Setup
-- ==========================================
--  Run this SQL in phpMyAdmin or MySQL CLI
-- ==========================================

CREATE DATABASE IF NOT EXISTS esp32farm;
USE esp32farm;

-- Farmers table (synced from SD card)
CREATE TABLE IF NOT EXISTS farmers (
    farmer_id VARCHAR(4) PRIMARY KEY,
    phone_number VARCHAR(20) NOT NULL,
    created_at DATETIME NOT NULL,
    synced_at DATETIME DEFAULT NULL
);

-- Soil readings (synced from SD card)
CREATE TABLE IF NOT EXISTS soil_readings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    farmer_id VARCHAR(4) NOT NULL,
    reading_timestamp VARCHAR(50) NOT NULL,
    humidity FLOAT DEFAULT NULL,
    temperature FLOAT DEFAULT NULL,
    ec FLOAT DEFAULT NULL,
    ph FLOAT DEFAULT NULL,
    nitrogen FLOAT DEFAULT NULL,
    phosphorus FLOAT DEFAULT NULL,
    potassium FLOAT DEFAULT NULL,
    synced_at DATETIME DEFAULT NULL,
    FOREIGN KEY (farmer_id) REFERENCES farmers(farmer_id) ON DELETE CASCADE
);

-- Sync requests (dashboard triggers, ESP32 polls)
CREATE TABLE IF NOT EXISTS sync_requests (
    id INT AUTO_INCREMENT PRIMARY KEY,
    requested_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME DEFAULT NULL,
    status ENUM('pending', 'completed', 'failed') DEFAULT 'pending'
);

-- SMS settings (configurable from dashboard, synced to ESP32 SD card)
CREATE TABLE IF NOT EXISTS sms_settings (
    id INT PRIMARY KEY DEFAULT 1,
    sms_enabled TINYINT(1) DEFAULT 0,
    message_template TEXT,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Default SMS template with placeholders
INSERT INTO sms_settings (id, sms_enabled, message_template) VALUES (
    1, 0,
    'Farm Report for ID:{farmer_id}\nMoisture:{humidity}%\nTemp:{temperature}C\npH:{ph}\nEC:{ec}\nN:{nitrogen} P:{phosphorus} K:{potassium}\nDate:{timestamp}'
) ON DUPLICATE KEY UPDATE id=id;

-- Index for faster queries
CREATE INDEX idx_readings_farmer ON soil_readings(farmer_id);
CREATE INDEX idx_readings_timestamp ON soil_readings(reading_timestamp);
CREATE INDEX idx_sync_status ON sync_requests(status);
