<?php
// ==========================================
//  TRIGGER SYNC API
//  GET: Check for pending sync requests (ESP32 polls this)
//  POST: Create a new sync request (dashboard button)
//  GET ?action=complete: Mark sync as completed
// ==========================================

require_once __DIR__ . '/../config.php';

$db = getDB();

try {
    $action = isset($_GET['action']) ? $_GET['action'] : '';

    // ---- ESP32 marks sync as complete ----
    if ($action === 'complete') {
        $status = isset($_GET['status']) ? $_GET['status'] : 'completed';

        $stmt = $db->prepare(
            "UPDATE sync_requests 
             SET status = :status, completed_at = NOW() 
             WHERE status = 'pending'"
        );
        $stmt->execute([':status' => $status]);

        jsonResponse(['success' => true, 'message' => "Sync marked as $status"]);
    }

    // ---- Dashboard creates sync request ----
    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        // Check if there's already a pending request
        $checkStmt = $db->query("SELECT id FROM sync_requests WHERE status = 'pending' LIMIT 1");

        if ($checkStmt->fetch()) {
            jsonResponse(['success' => true, 'message' => 'Sync request already pending']);
        }

        $stmt = $db->prepare(
            "INSERT INTO sync_requests (requested_at, status) VALUES (NOW(), 'pending')"
        );
        $stmt->execute();

        jsonResponse([
            'success' => true,
            'message' => 'Sync request created. Waiting for ESP32 to connect and sync.',
            'request_id' => $db->lastInsertId()
        ]);
    }

    // ---- ESP32 polls for pending sync requests (GET) ----
    $stmt = $db->query(
        "SELECT id, requested_at, status FROM sync_requests 
         WHERE status = 'pending' 
         ORDER BY requested_at DESC LIMIT 1"
    );
    $pending = $stmt->fetch();

    // Also get last completed sync info
    $lastSync = $db->query(
        "SELECT completed_at, status FROM sync_requests 
         WHERE status IN ('completed', 'failed') 
         ORDER BY completed_at DESC LIMIT 1"
    )->fetch();

    jsonResponse([
        'success' => true,
        'sync_pending' => $pending ? true : false,
        'pending_request' => $pending,
        'last_sync' => $lastSync
    ]);

} catch (Exception $e) {
    jsonResponse(['success' => false, 'message' => $e->getMessage()], 500);
}
?>