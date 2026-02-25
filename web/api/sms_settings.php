<?php
// ==========================================
//  SMS SETTINGS API
//  GET: Retrieve current SMS settings
//  POST: Update SMS settings
// ==========================================

require_once __DIR__ . '/../config.php';

$db = getDB();

try {
    // ---- GET: Retrieve settings ----
    if ($_SERVER['REQUEST_METHOD'] === 'GET') {
        $stmt = $db->query("SELECT sms_enabled, message_template, updated_at FROM sms_settings WHERE id = 1");
        $settings = $stmt->fetch();

        if (!$settings) {
            // Insert default if missing
            $db->exec(
                "INSERT INTO sms_settings (id, sms_enabled, message_template) VALUES (
                    1, 0,
                    'Farm Report for ID:{farmer_id}\nMoisture:{humidity}%\nTemp:{temperature}C\npH:{ph}\nEC:{ec}\nN:{nitrogen} P:{phosphorus} K:{potassium}\nDate:{timestamp}'
                ) ON DUPLICATE KEY UPDATE id=id"
            );
            $stmt = $db->query("SELECT sms_enabled, message_template, updated_at FROM sms_settings WHERE id = 1");
            $settings = $stmt->fetch();
        }

        jsonResponse([
            'success' => true,
            'settings' => [
                'sms_enabled' => (bool)$settings['sms_enabled'],
                'message_template' => $settings['message_template'],
                'updated_at' => $settings['updated_at']
            ]
        ]);
    }

    // ---- POST: Update settings ----
    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        $rawInput = file_get_contents('php://input');
        $data = json_decode($rawInput, true);

        if ($data === null) {
            jsonResponse(['success' => false, 'message' => 'Invalid JSON payload'], 400);
        }

        $enabled = isset($data['sms_enabled']) ? ($data['sms_enabled'] ? 1 : 0) : 0;
        $template = isset($data['message_template']) ? trim($data['message_template']) : '';

        if (empty($template)) {
            jsonResponse(['success' => false, 'message' => 'Message template cannot be empty'], 400);
        }

        $stmt = $db->prepare(
            "INSERT INTO sms_settings (id, sms_enabled, message_template) 
             VALUES (1, :enabled, :template)
             ON DUPLICATE KEY UPDATE 
                sms_enabled = VALUES(sms_enabled),
                message_template = VALUES(message_template)"
        );
        $stmt->execute([
            ':enabled' => $enabled,
            ':template' => $template
        ]);

        jsonResponse([
            'success' => true,
            'message' => 'SMS settings saved successfully'
        ]);
    }

    jsonResponse(['success' => false, 'message' => 'Method not allowed'], 405);

} catch (Exception $e) {
    jsonResponse(['success' => false, 'message' => $e->getMessage()], 500);
}
?>
