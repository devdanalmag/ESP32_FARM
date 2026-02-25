<?php
// ==========================================
//  READINGS API - Soil reading data
//  GET: List readings with optional filters
// ==========================================

require_once __DIR__ . '/../config.php';

$db = getDB();

try {
    $farmerId = isset($_GET['farmer_id']) ? trim($_GET['farmer_id']) : '';
    $limit = isset($_GET['limit']) ? min(intval($_GET['limit']), 500) : 100;
    $offset = isset($_GET['offset']) ? intval($_GET['offset']) : 0;

    $where = "";
    $params = [];

    if ($farmerId) {
        $where = "WHERE s.farmer_id = :farmer_id";
        $params[':farmer_id'] = $farmerId;
    }

    // Get readings with farmer info
    $sql = "SELECT s.*, f.phone_number 
            FROM soil_readings s 
            JOIN farmers f ON s.farmer_id = f.farmer_id 
            $where 
            ORDER BY s.id DESC 
            LIMIT $limit OFFSET $offset";

    $stmt = $db->prepare($sql);
    $stmt->execute($params);
    $readings = $stmt->fetchAll();

    // Get total count
    $countSql = "SELECT COUNT(*) as total FROM soil_readings s $where";
    $countStmt = $db->prepare($countSql);
    $countStmt->execute($params);
    $total = $countStmt->fetch()['total'];

    // Get summary stats
    $statsSql = "SELECT 
                    COUNT(*) as total_readings,
                    COUNT(DISTINCT farmer_id) as unique_farmers,
                    AVG(humidity) as avg_humidity,
                    AVG(temperature) as avg_temperature,
                    AVG(ec) as avg_ec,
                    AVG(ph) as avg_ph,
                    AVG(nitrogen) as avg_nitrogen,
                    AVG(phosphorus) as avg_phosphorus,
                    AVG(potassium) as avg_potassium
                 FROM soil_readings s $where";
    $statsStmt = $db->prepare($statsSql);
    $statsStmt->execute($params);
    $stats = $statsStmt->fetch();

    jsonResponse([
        'success' => true,
        'total' => (int) $total,
        'stats' => $stats,
        'readings' => $readings
    ]);

} catch (Exception $e) {
    jsonResponse(['success' => false, 'message' => $e->getMessage()], 500);
}
?>