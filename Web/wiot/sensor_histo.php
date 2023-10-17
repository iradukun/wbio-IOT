<html>
<head>
<style>
table, th, td {
  border: 1px solid black;
  border-collapse: collapse;
}
th, td {
    padding : 8px;
}
</style>
</head>
<body>
<?php

    if (!isset($_GET['deviceid']))
    {
?>
<h1>Error</h1>Device id is not present
<?php
    die();
    }
    $deviceid=$_GET['deviceid'];
?>
<h1>Historical data</h1>
Sensor&nbsp;:&nbsp;<b><?php echo "$deviceid"?></b><br><br>

<table>
<thead>
<tr>
    <th>Extra data</th>
    <th>sensor datetime</th>
    <th>server datetime</th>
    <th>priority</th>
    <th>longitude</th>
    <th>latitude</th>
    <th>altitude</th>
    <th>angle</th>
    <th>satellites</th>
    <th>speed</th>
</tr>
</thead>
</tbody>
<?php
    require "/usr/lib/composer/vendor/autoload.php";
    $client = new MongoDB\Client("mongodb://localhost:27017");
    
    $collection_name="sensor_".$deviceid;
    $collection = $client->WioT_Mobility->$collection_name;

    $result = $collection->find();

    foreach ($result as $entry)
    {
        echo "<tr>";
        echo "<td style=\"text-align:center\"><a href=\"sensor_detail.php?deviceid=".$deviceid."&oid=".$entry['_id']."\"><img width=\"32\" height=\"32\" src=\"detail.png\"></a></td>";
        echo "<td>".$entry['time_sensor']->toDateTime()->format('Y-m-d H:i:s')."</td>";
        echo "<td>".$entry['time_server']->toDateTime()->format('Y-m-d H:i:s')."</td>";
        echo "<td>".$entry['priority']."</td>";
        echo "<td>".$entry['longitude']."</td>";
        echo "<td>".$entry['latitude']."</td>";
        echo "<td>".$entry['altitude']."</td>";
        echo "<td>".$entry['angle']."</td>";
        echo "<td>".$entry['satellites']."</td>";
        echo "<td>".$entry['speed']."</td>";
        echo "</tr>";
    }
 
?>
</tbody>
</table>
</body>
</html>
