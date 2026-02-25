<?php
// ==========================================
//  FARMERS API - List and search farmers
//  GET: List all farmers or search by ID
// ==========================================

require_once __DIR__ . '/../config.php';

$db = getDB();

try {
    $search = isset($_GET['search']) ? trim($_GET['search']) : '';

    if ($search) {
        $stmt = $db->prepare(
            "SELECT f.*, 
                    COUNT(s.id) as reading_count,
                    MAX(s.reading_timestamp) as last_reading
             FROM farmers f 
             LEFT JOIN soil_readings s ON f.farmer_id = s.farmer_id 
             WHERE f.farmer_id LIKE :search OR f.phone_number LIKE :search2
             GROUP BY f.farmer_id 
             ORDER BY f.farmer_id"
        );
        $stmt->execute([
            ':search' => "%$search%",
            ':search2' => "%$search%"
        ]);
    } else {
        $stmt = $db->query(
            "SELECT f.*, 
                    COUNT(s.id) as reading_count,
                    MAX(s.reading_timestamp) as last_reading
             FROM farmers f 
             LEFT JOIN soil_readings s ON f.farmer_id = s.farmer_id 
             GROUP BY f.farmer_id 
             ORDER BY f.farmer_id"
        );
    }

    $farmers = $stmt->fetchAll();

    // Get total count
    $totalStmt = $db->query("SELECT COUNT(*) as total FROM farmers");
    $total = $totalStmt->fetch()['total'];

    jsonResponse([
        'success' => true,
        'total' => (int) $total,
        'farmers' => $farmers
    ]);

} catch (Exception $e) {
    jsonResponse(['success' => false, 'message' => $e->getMessage()], 500);
}
?>