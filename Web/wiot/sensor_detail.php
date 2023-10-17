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

    if (!isset($_GET['deviceid']) ||
        !isset($_GET['oid']))
    {
?>
<h1>Error</h1>Device or data id is not present
<?php
    die();
    }
    
    require_once("IOT_table.php");
    
    $deviceid=$_GET['deviceid'];
    $oid=$_GET['oid'];
    
    require "/usr/lib/composer/vendor/autoload.php";
    $client = new MongoDB\Client("mongodb://localhost:27017");
    
    $collection_name="sensor_".$deviceid;
    $collection = $client->WioT_Mobility->$collection_name;

    $result = $collection->findOne(['_id'=> new MongoDB\BSON\ObjectId("$oid")],
                                   [ 'projection' => ['time_sensor' => 1, 'data' => 1] ]);

?>
<h1>Detailled data</h1>
Sensor&nbsp;:&nbsp;<b><?php echo "$deviceid"?></b><br>
Date&nbsp;:&nbsp;<?php echo $result['time_sensor']->toDateTime()->format('Y-m-d H:i:s')?><br><br>

<?php
    if ($result===null)
    {
        echo "No data found";
        die();
    }
?>
<table>
<thead>
<tr>
    <th>Data ID</th>
    <th>Desc</th>
    <th>Value</th>
</tr>
</thead>
</tbody>
<?php
    foreach ($result['data'] as $key=>$value)
    {
        preg_match('/io#(\d*)/', $key, $matches);

        $iot_idx=0+$matches[1];
    
        if (isset($g_IOT[$iot_idx]))
        {
            $libelle=$g_IOT[$iot_idx]['title'];
            if (isset($g_IOT[$iot_idx]['scale']))
            {
                $value*=$g_IOT[$iot_idx]['scale'];
            }
            if (isset($g_IOT[$iot_idx]['unit']))
            {
                $value.='&nbsp;'.$g_IOT[$iot_idx]['unit'];
            }
        }
        else
        {
            $libelle="&nbsp;".$iot_idx;
        }
        echo "<tr>";
        echo "<td>".$key."</td>";
        echo "<td>$libelle</td>";
        echo "<td>$value</td>";
        echo "</tr>";
    }
?>
</tbody>
</table>
</body>
</html>
