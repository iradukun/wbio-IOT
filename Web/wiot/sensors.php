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
<H1>List of available sensors</H1>

<table>
<thead>
<tr>
    <th>IMEI</th>
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
    
    $collection = $client->WioT_Mobility->sensors;

    $result = $collection->find();

    foreach ($result as $entry)
    {
        echo "<tr>";
        echo "<td><a href=\"sensor_histo.php?deviceid=".$entry['IMEI']."\"><b>".$entry['IMEI']."</b></a></td>";
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
