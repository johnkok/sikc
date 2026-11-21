@file:OptIn(ExperimentalMaterial3Api::class)

package com.kokkoris.irrigation

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val repo = MqttRepository(this)
        setContent {
            IrrigationTheme {
                App(repo)
            }
        }
    }
}

@Composable
fun IrrigationTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = lightColorScheme(
            primary = Color(0xFF246B45),
            secondary = Color(0xFF53634F),
            surface = Color(0xFFF8FBF7),
            error = Color(0xFFBA1A1A)
        ),
        content = content
    )
}

@Composable
fun App(repo: MqttRepository) {
    var tab by remember { mutableIntStateOf(0) }
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Irrigation Control") },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color(0xFFE1F1E4))
            )
        },
        bottomBar = {
            NavigationBar {
                listOf("Status", "Plans", "Connection").forEachIndexed { i, n ->
                    NavigationBarItem(
                        selected = tab == i,
                        onClick = { tab = i },
                        icon = { Text(if (i == 0) "●" else if (i == 1) "▦" else "⚙") },
                        label = { Text(n) }
                    )
                }
            }
        }
    ) { pad ->
        Box(Modifier.padding(pad)) {
            when (tab) {
                0 -> StatusScreen(repo)
                1 -> PlansScreen(repo)
                else -> ConnectionScreen(repo)
            }
        }
    }
}

@Composable
fun StatusScreen(repo: MqttRepository) {
    val s = repo.state
    Column(
        Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                if (s.connected) "Connected" else "Offline",
                fontWeight = FontWeight.Bold,
                color = if (s.connected) Color(0xFF167542) else MaterialTheme.colorScheme.error
            )
            Spacer(Modifier.weight(1f))
            Text(repo.message)
        }
        ElevatedCard(Modifier.fillMaxWidth()) {
            Column(Modifier.padding(16.dp)) {
                Text("Water system", fontWeight = FontWeight.Bold)
                Text("Tank: ${s.tankState}")
                Text("Well pump: ${if (s.wellPump) "ON" else "OFF"}")
                Text("Irrigation flow: %.2f L/min".format(s.irrigationLpm))
                Text("Well flow: %.2f L/min".format(s.wellLpm))
                Text("Consumed: %.1f L   Refilled: %.1f L".format(s.irrigationLiters, s.wellLiters))
            }
        }
        if (s.faults != 0) {
            ElevatedCard(colors = CardDefaults.elevatedCardColors(containerColor = Color(0xFFFFDAD6))) {
                Column(Modifier.padding(16.dp)) {
                    Text("Fault active: ${faultText(s.faults)}", fontWeight = FontWeight.Bold)
                    Button(onClick = repo::clearFaults) {
                        Text("Clear after physical fault is fixed")
                    }
                }
            }
        }
        Text("Zones", style = MaterialTheme.typography.titleMedium)
        s.zones.forEachIndexed { i, on ->
            ElevatedCard(Modifier.fillMaxWidth()) {
                Row(
                    Modifier.padding(12.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column(Modifier.weight(1f)) {
                        Text("Zone ${i + 1}", fontWeight = FontWeight.Bold)
                        Text("Soil ${s.soilMoisture.getOrNull(i)?.let { "%.1f%%".format(it) } ?: "—"}")
                    }
                    Switch(
                        checked = on,
                        onCheckedChange = { repo.setZone(i, it) },
                        enabled = s.connected && s.faults == 0
                    )
                }
            }
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Weather/rain block", Modifier.weight(1f))
            Switch(s.weatherBlock, { repo.setWeatherBlock(it) }, enabled = s.connected)
        }
        DisplayBrightnessCard(repo, s.displayBrightness)
    }
}

@Composable
fun DisplayBrightnessCard(repo: MqttRepository, currentBrightness: Int) {
    var brightness by remember(currentBrightness) { mutableFloatStateOf(currentBrightness.toFloat()) }
    ElevatedCard(Modifier.fillMaxWidth()) {
        Column(
            Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text("Status display", fontWeight = FontWeight.Bold)
            Text("Brightness: ${brightness.toInt()}%")
            Slider(
                value = brightness,
                onValueChange = { brightness = it },
                valueRange = 0f..100f,
                steps = 19,
                enabled = repo.state.connected
            )
            Text(
                "The controller starts at 8%. Higher settings require a suitably rated external 5 V supply.",
                style = MaterialTheme.typography.bodySmall
            )
            Button(
                onClick = { repo.setDisplayBrightness(brightness.toInt()) },
                enabled = repo.state.connected,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Save display brightness")
            }
        }
    }
}

@Composable
fun PlansScreen(repo: MqttRepository) {
    Column(
        Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Plans are saved in the KC868-A16 and run without the phone.")
        repo.state.plans.forEachIndexed { i, p ->
            PlanCard(i, p, repo)
        }
    }
}

@Composable
fun PlanCard(index: Int, initial: ZonePlan, repo: MqttRepository) {
    var p by remember(initial) { mutableStateOf(initial) }
    var expanded by remember { mutableStateOf(index == 0) }
    ElevatedCard(Modifier.fillMaxWidth()) {
        Column(
            Modifier.padding(14.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("Zone ${index + 1}", fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
                TextButton(onClick = { expanded = !expanded }) {
                    Text(if (expanded) "Hide" else "Edit")
                }
            }
            if (expanded) {
                LabeledSwitch("Automatic moisture control", p.automatic) { p = p.copy(automatic = it) }
                LabeledSwitch("Daily/weekly schedule", p.scheduleEnabled) { p = p.copy(scheduleEnabled = it) }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    NumberField("Start %", p.startMoisture.toString(), Modifier.weight(1f)) {
                        p = p.copy(startMoisture = it.toFloatOrNull() ?: p.startMoisture)
                    }
                    NumberField("Stop %", p.stopMoisture.toString(), Modifier.weight(1f)) {
                        p = p.copy(stopMoisture = it.toFloatOrNull() ?: p.stopMoisture)
                    }
                }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    NumberField("Hour", p.hour.toString(), Modifier.weight(1f)) {
                        p = p.copy(hour = (it.toIntOrNull() ?: p.hour).coerceIn(0, 23))
                    }
                    NumberField("Minute", p.minute.toString(), Modifier.weight(1f)) {
                        p = p.copy(minute = (it.toIntOrNull() ?: p.minute).coerceIn(0, 59))
                    }
                    NumberField("Duration min", p.durationMin.toString(), Modifier.weight(1.4f)) {
                        p = p.copy(durationMin = (it.toIntOrNull() ?: p.durationMin).coerceIn(1, 1440))
                    }
                }
                Text("Days")
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    listOf("S", "M", "T", "W", "T", "F", "S").forEachIndexed { d, n ->
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(n)
                            Checkbox(
                                checked = p.days and (1 shl d) != 0,
                                onCheckedChange = {
                                    p = p.copy(
                                        days = if (it) p.days or (1 shl d) else p.days and (1 shl d).inv()
                                    )
                                }
                            )
                        }
                    }
                }
                NumberField("Maximum runtime (minutes)", p.maxRuntimeMin.toString(), Modifier.fillMaxWidth()) {
                    p = p.copy(maxRuntimeMin = (it.toIntOrNull() ?: p.maxRuntimeMin).coerceIn(1, 1440))
                }
                Button(
                    onClick = { repo.savePlan(index, p) },
                    enabled = repo.state.connected,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Save Zone ${index + 1} plan")
                }
            }
        }
    }
}

@Composable
fun ConnectionScreen(repo: MqttRepository) {
    var c by remember { mutableStateOf(repo.config) }
    Column(
        Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text("MQTT connection", style = MaterialTheme.typography.titleLarge)
        Text("Use the same local broker and topic prefix as the KC868-A16 firmware.")
        TextField(c.host, { c = c.copy(host = it) }, label = { Text("Broker IP/hostname") }, modifier = Modifier.fillMaxWidth())
        NumberField("Port", c.port.toString(), Modifier.fillMaxWidth()) {
            c = c.copy(port = it.toIntOrNull() ?: 1883)
        }
        TextField(c.username, { c = c.copy(username = it) }, label = { Text("Username") }, modifier = Modifier.fillMaxWidth())
        TextField(c.password, { c = c.copy(password = it) }, label = { Text("Password") }, visualTransformation = PasswordVisualTransformation(), modifier = Modifier.fillMaxWidth())
        TextField(c.prefix, { c = c.copy(prefix = it.trim('/')) }, label = { Text("Topic prefix") }, modifier = Modifier.fillMaxWidth())
        LabeledSwitch("TLS", c.tls) { c = c.copy(tls = it) }
        Button(onClick = { repo.connect(c) }, modifier = Modifier.fillMaxWidth()) {
            Text("Save and connect")
        }
        OutlinedButton(onClick = repo::disconnect, modifier = Modifier.fillMaxWidth()) {
            Text("Disconnect")
        }
    }
}

@Composable
fun LabeledSwitch(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, Modifier.weight(1f))
        Switch(checked, onChange)
    }
}

@Composable
fun NumberField(label: String, value: String, modifier: Modifier = Modifier, onChange: (String) -> Unit) {
    var text by remember(value) { mutableStateOf(value) }
    OutlinedTextField(
        text,
        {
            text = it
            onChange(it)
        },
        label = { Text(label) },
        singleLine = true,
        modifier = modifier
    )
}

fun faultText(mask: Int) = listOf(
    1 to "tank sensors",
    2 to "refill timeout",
    4 to "well overload",
    8 to "dry well",
    16 to "well no-flow",
    32 to "irrigation no-flow",
    64 to "leak",
    128 to "soil sensor"
).filter { mask and it.first != 0 }.joinToString { it.second }
