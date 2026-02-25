<?php
// ==========================================
//  SYNC API - Receives data from ESP32
//  POST: Upload farmers + datalog CSV data
// ==========================================

require_once __DIR__ . '/../config.php';

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonResponse(['success' => false, 'message' => 'POST method required'], 405);
}

// Read JSON payload
$rawInput = file_get_contents('php://input');
$data = json_decode($rawInput, true);

if (!$data || !isset($data['farmers_csv']) || !isset($data['datalog_csv'])) {
    jsonResponse(['success' => false, 'message' => 'Missing farmers_csv or datalog_csv in payload'], 400);
}

$db = getDB();

try {
    $db->beginTransaction();

    $farmersImported = 0;
    $readingsImported = 0;
    $now = date('Y-m-d H:i:s');

    // ---- Process Farmers CSV ----
    $farmersLines = explode("\n", trim($data['farmers_csv']));

    // Skip header line
    for ($i = 1; $i < count($farmersLines); $i++) {
        $line = trim($farmersLines[$i]);
        if (empty($line))
            continue;

        $fields = str_getcsv($line);
        if (count($fields) < 3)
            continue;

        $farmerId = trim($fields[0]);
        $phone = trim($fields[1]);
        $createdAt = trim($fields[2]);

        // Upsert farmer (insert or update on duplicate)
        $stmt = $db->prepare(
            "INSERT INTO farmers (farmer_id, phone_number, created_at, synced_at) 
             VALUES (:id, :phone, :created, :synced) 
             ON DUPLICATE KEY UPDATE 
                phone_number = VALUES(phone_number),
                synced_at = VALUES(synced_at)"
        );
        $stmt->execute([
            ':id' => $farmerId,
            ':phone' => $phone,
            ':created' => $createdAt,
            ':synced' => $now
        ]);
        $farmersImported++;
    }

    // ---- Process Datalog CSV ----
    $datalogLines = explode("\n", trim($data['datalog_csv']));

    // Skip header line
    for ($i = 1; $i < count($datalogLines); $i++) {
        $line = trim($datalogLines[$i]);
        if (empty($line))
            continue;

        $fields = str_getcsv($line);
        if (count($fields) < 9)
            continue;

        $farmerId = trim($fields[0]);
        $timestamp = trim($fields[1]);
        $humidity = floatval($fields[2]);
        $temperature = floatval($fields[3]);
        $ec = floatval($fields[4]);
        $ph = floatval($fields[5]);
        $nitrogen = floatval($fields[6]);
        $phosphorus = floatval($fields[7]);
        $potassium = floatval($fields[8]);

        // Check if this exact reading already exists (prevent duplicates)
        $checkStmt = $db->prepare(
            "SELECT id FROM soil_readings 
             WHERE farmer_id = :id AND reading_timestamp = :ts 
             LIMIT 1"
        );
        $checkStmt->execute([':id' => $farmerId, ':ts' => $timestamp]);

        if (!$checkStmt->fetch()) {
            // Insert new reading
            $stmt = $db->prepare(
                "INSERT INTO soil_readings 
                 (farmer_id, reading_timestamp, humidity, temperature, ec, ph, nitrogen, phosphorus, potassium, synced_at) 
                 VALUES (:id, :ts, :h, :t, :ec, :ph, :n, :p, :k, :synced)"
            );
            $stmt->execute([
                ':id' => $farmerId,
                ':ts' => $timestamp,
                ':h' => $humidity,
                ':t' => $temperature,
                ':ec' => $ec,
                ':ph' => $ph,
                ':n' => $nitrogen,
                ':p' => $phosphorus,
                ':k' => $potassium,
                ':synced' => $now
            ]);
            $readingsImported++;
        }
    }

    // Mark any pending sync requests as completed
    $db->exec("UPDATE sync_requests SET status = 'completed', completed_at = NOW() WHERE status = 'pending'");

    $db->commit();

    // Fetch SMS settings to send back to ESP32
    $smsStmt = $db->query("SELECT sms_enabled, message_template FROM sms_settings WHERE id = 1");
    $smsSettings = $smsStmt->fetch();

    $smsData = [
        'enabled' => false,
        'template' => ''
    ];
    if ($smsSettings) {
        $smsData['enabled'] = (bool) $smsSettings['sms_enabled'];
        $smsData['template'] = $smsSettings['message_template'];
    }

    jsonResponse([
        'success' => true,
        'message' => "Sync complete. Farmers: $farmersImported, Readings: $readingsImported",
        'farmers_imported' => $farmersImported,
        'readings_imported' => $readingsImported,
        'sms_settings' => $smsData,
        'server_time' => [
            'year' => (int) date('Y'),
            'month' => (int) date('n'),
            'day' => (int) date('j'),
            'hour' => (int) date('G'),
            'minute' => (int) date('i'),
            'second' => (int) date('s')
        ]
    ]);

} catch (Exception $e) {
    $db->rollBack();
    jsonResponse(['success' => false, 'message' => 'Sync failed: ' . $e->getMessage()], 500);
}
?>