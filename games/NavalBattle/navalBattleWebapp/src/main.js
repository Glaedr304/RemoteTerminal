import "./style.css";

// === DOM References ===
const userIdInput = document.getElementById("userid");
const gameIdInput = document.getElementById("gameid");
const connectBtn = document.getElementById("connectBtn");
const statusSpan = document.getElementById("status");
const statusBadge = document.getElementById("statusBadge");
const logPre = document.getElementById("log");

const connectSection = document.getElementById("connect");
const gameSection = document.getElementById("game");

const meSpan = document.getElementById("me");
const opponentSpan = document.getElementById("opponent");
const phaseSpan = document.getElementById("phase");
const turnIndicator = document.getElementById("turnIndicator");
const youReadyBadge = document.getElementById("youReadyBadge");
const opponentReadyBadge = document.getElementById("opponentReadyBadge");

const setupControls = document.getElementById("setupControls");
const rotateBtn = document.getElementById("rotateBtn");
const readyBtn = document.getElementById("readyBtn");

const messageToast = document.getElementById("messageToast");
const messageText = document.getElementById("messageText");

const gameOverOverlay = document.getElementById("gameOverOverlay");
const gameOverTitle = document.getElementById("gameOverTitle");
const gameOverMessage = document.getElementById("gameOverMessage");
const rematchBtn = document.getElementById("rematchBtn");
const homeBtn = document.getElementById("homeBtn");

const ownGrid = document.getElementById("ownGrid");
const oppGrid = document.getElementById("oppGrid");
const ownColLabels = document.getElementById("ownColLabels");
const ownRowLabels = document.getElementById("ownRowLabels");
const oppColLabels = document.getElementById("oppColLabels");
const oppRowLabels = document.getElementById("oppRowLabels");

const yourFleetList = document.getElementById("yourFleetList");
const opponentFleetList = document.getElementById("opponentFleetList");

// === State ===
let socket = null;
let rotation = 0;
let lastSetupInfo = null;
let messageTimeout = null;
let lastPhase = null;
let placedShipIds = new Set();
let placedPlaneIds = new Set();
let hoveredCell = null; // Track currently hovered cell for preview refresh
let myUserId = null;
let gameId = null;
let rematchRequested = false;
let opponentWantsRematch = false;
let selectedVehicle = null; // { type: 'ship' | 'plane', id: number }
let activeAbility = null; // { vehicleId: number, type: string, firingPattern: string | null }

// Ability configuration - what additional options each ability needs
const ABILITY_CONFIG = {
    torpedo: { needsTarget: true, needsFiringPattern: true, patterns: ["vertical", "horizontal"], targetType: "opponent" },
    exocet: { needsTarget: true, needsFiringPattern: false, targetType: "opponent" },
    apache: { needsTarget: true, needsFiringPattern: true, patterns: ["vertical", "horizontal"], targetType: "opponent" },
    tomahawk: { needsTarget: true, needsFiringPattern: true, patterns: ["plus", "x"], targetType: "opponent" },
    scan: { needsTarget: true, needsFiringPattern: false, targetType: "opponent" },
    reveal: { needsTarget: true, needsFiringPattern: true, patterns: ["square", "diamond"], targetType: "opponent" },
    relocate: { needsTarget: true, needsFiringPattern: false, targetType: "own", needsShipId: true }
};

// === Utility Functions ===
function logLine(text) {
    const timestamp = new Date().toLocaleTimeString();
    logPre.textContent += `[${timestamp}] ${text}\n`;
    logPre.scrollTop = logPre.scrollHeight;
}

function showMessage(text, type = "info") {
    messageText.textContent = text;
    messageToast.className = `message-toast ${type}`;
    messageToast.classList.remove("hidden");
    
    if (messageTimeout) clearTimeout(messageTimeout);
    messageTimeout = setTimeout(() => {
        messageToast.classList.add("hidden");
    }, 3500);
}

function setConnectionStatus(status) {
    statusSpan.textContent = status.charAt(0).toUpperCase() + status.slice(1);
    statusBadge.className = `status-badge ${status}`;
}

function updatePhaseDisplay(phase) {
    phaseSpan.textContent = phase;
    phaseSpan.className = `phase-badge ${phase}`;
    
    // Show/hide setup controls based on phase
    if (phase === "setup") {
        setupControls.classList.remove("hidden");
    } else {
        setupControls.classList.add("hidden");
    }
}

function updateTurnIndicator(currentTurn, myUserId) {
    if (!currentTurn) {
        turnIndicator.classList.add("hidden");
        return;
    }
    
    turnIndicator.classList.remove("hidden");
    
    if (currentTurn === myUserId) {
        turnIndicator.textContent = "Your Turn";
        turnIndicator.className = "turn-indicator";
    } else {
        turnIndicator.textContent = "Opponent's Turn";
        turnIndicator.className = "turn-indicator not-your-turn";
    }
}

function updateReadyStatus(youReady, opponentReady) {
    youReadyBadge.classList.toggle("hidden", !youReady);
    opponentReadyBadge.classList.toggle("hidden", !opponentReady);
    readyBtn.classList.toggle("is-ready", youReady);
}

function showGameOver(isVictory) {
    gameOverOverlay.classList.remove("hidden");
    const content = gameOverOverlay.querySelector(".game-over-content");
    
    // Reset rematch state
    rematchRequested = false;
    opponentWantsRematch = false;
    updateRematchButton();
    
    if (isVictory) {
        content.classList.add("victory");
        content.classList.remove("defeat");
        gameOverTitle.textContent = "Victory!";
        gameOverMessage.textContent = "You sank all enemy ships!";
    } else {
        content.classList.add("defeat");
        content.classList.remove("victory");
        gameOverTitle.textContent = "Defeat";
        gameOverMessage.textContent = "Your fleet has been destroyed.";
    }
}

function updateRematchButton() {
    if (rematchRequested && opponentWantsRematch) {
        rematchBtn.textContent = "Starting Rematch...";
        rematchBtn.disabled = true;
    } else if (rematchRequested) {
        rematchBtn.textContent = "Waiting for opponent...";
        rematchBtn.disabled = true;
    } else if (opponentWantsRematch) {
        rematchBtn.textContent = "Accept Rematch";
        rematchBtn.disabled = false;
    } else {
        rematchBtn.textContent = "Rematch";
        rematchBtn.disabled = false;
    }
}

function hideGameOver() {
    gameOverOverlay.classList.add("hidden");
}

// === Grid Building ===
function buildGridLabels(colContainer, rowContainer, rows, cols) {
    colContainer.innerHTML = "";
    rowContainer.innerHTML = "";
    
    // Column labels: 1, 2, 3, ...
    for (let c = 0; c < cols; c++) {
        const label = document.createElement("span");
        label.textContent = String(c + 1);
        colContainer.appendChild(label);
    }
    
    // Row labels: A, B, C, ...
    for (let r = 0; r < rows; r++) {
        const label = document.createElement("span");
        label.textContent = String.fromCharCode(65 + r); // A=65
        rowContainer.appendChild(label);
    }
}

function buildGrids(rows, cols) {
    ownGrid.style.setProperty("--rows", rows);
    ownGrid.style.setProperty("--cols", cols);
    oppGrid.style.setProperty("--rows", rows);
    oppGrid.style.setProperty("--cols", cols);

    // Build labels
    buildGridLabels(ownColLabels, ownRowLabels, rows, cols);
    buildGridLabels(oppColLabels, oppRowLabels, rows, cols);

    ownGrid.innerHTML = "";
    oppGrid.innerHTML = "";

    // Build own grid
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const cell = document.createElement("div");
            cell.className = "cell";
            cell.dataset.row = r;
            cell.dataset.col = c;

            cell.addEventListener("click", () => handleOwnGridClick(r, c));
            cell.addEventListener("mouseenter", () => handleOwnGridHover(r, c));
            cell.addEventListener("mouseleave", () => clearPreviewAndHover());
            ownGrid.appendChild(cell);
        }
    }

    // Build opponent grid
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const cell = document.createElement("div");
            cell.className = "cell";
            cell.dataset.row = r;
            cell.dataset.col = c;

            cell.addEventListener("click", () => handleOppGridClick(r, c));
            oppGrid.appendChild(cell);
        }
    }
}

// === Fleet Panel Rendering ===
function getAbilityIcon(abilityType) {
    const icons = {
        torpedo: "\u{1F4A5}",      // ?? explosion
        exocet: "\u{1F680}",       // ?? rocket
        apache: "\u{1F681}",       // ?? helicopter
        tomahawk: "\u2604\uFE0F",  // ?? comet/meteor
        scan: "\u{1F50D}",         // ?? magnifying glass
        reveal: "\u{1F441}\uFE0F", // ??? eye
        relocate: "\u21C4"         // ? arrows
    };
    return icons[abilityType] || "\u2753";
}

function getAbilityDisplayName(abilityType) {
    const names = {
        torpedo: "Torpedo",
        exocet: "Exocet",
        apache: "Apache",
        tomahawk: "Tomahawk",
        scan: "Scan",
        reveal: "Reveal",
        relocate: "Relocate"
    };
    return names[abilityType] || abilityType;
}

function calculateShipBounds(coords) {
    if (!coords || coords.length === 0) return { minRow: 0, maxRow: 0, minCol: 0, maxCol: 0, rows: 1, cols: 1 };

    let minRow = Infinity, maxRow = -Infinity;
    let minCol = Infinity, maxCol = -Infinity;

    for (const c of coords) {
        if (c.row < minRow) minRow = c.row;
        if (c.row > maxRow) maxRow = c.row;
        if (c.col < minCol) minCol = c.col;
        if (c.col > maxCol) maxCol = c.col;
    }

    return {
        minRow,
        maxRow,
        minCol,
        maxCol,
        rows: maxRow - minRow + 1,
        cols: maxCol - minCol + 1
    };
}

function createShipFormGrid(coords) {
    const bounds = calculateShipBounds(coords);
    const container = document.createElement("div");
    container.className = "ship-form";

    const grid = document.createElement("div");
    grid.className = "ship-form-grid";
    grid.style.gridTemplateColumns = `repeat(${bounds.cols}, 12px)`;
    grid.style.gridTemplateRows = `repeat(${bounds.rows}, 12px)`;

    // Create a set for quick lookup
    const coordSet = new Set(coords.map(c => `${c.row},${c.col}`));

    // Fill grid
    for (let r = bounds.minRow; r <= bounds.maxRow; r++) {
        for (let c = bounds.minCol; c <= bounds.maxCol; c++) {
            const cell = document.createElement("div");
            cell.className = "ship-form-cell";
            if (coordSet.has(`${r},${c}`)) {
                cell.classList.add("ship");
            } else {
                cell.classList.add("empty");
            }
            grid.appendChild(cell);
        }
    }

    container.appendChild(grid);
    return container;
}

function createAbilityButton(ability, shipId, isYours) {
    const element = document.createElement(isYours ? "button" : "div");
    element.className = isYours ? "ability-btn" : "ability-display";
    element.dataset.abilityType = ability.type;
    element.dataset.vehicleId = shipId;

    const icon = document.createElement("span");
    icon.className = "ability-icon";
    icon.textContent = getAbilityIcon(ability.type);

    const name = document.createElement("span");
    name.className = "ability-name";
    name.textContent = getAbilityDisplayName(ability.type);

    const uses = document.createElement("span");
    uses.className = "ability-uses";
    if (ability.usagepolicy === "unlimited") {
        uses.textContent = "\u221E"; // ? infinity symbol
        uses.classList.add("unlimited");
    } else {
        uses.textContent = `${ability.remaininguses}`;
    }

    element.appendChild(icon);
    element.appendChild(name);
    element.appendChild(uses);

    if (isYours) {
        element.disabled = !ability.canuse;
        element.addEventListener("click", () => handleAbilityClick(shipId, ability.type));
    }

    return element;
}

function createShipCard(ship, isYours) {
    const card = document.createElement("div");
    card.className = "ship-card";
    card.dataset.shipId = ship.id;

    if (ship.issunk) {
        card.classList.add("sunk");
    }

    // Mark as placed if ship has valid position
    const isPlaced = ship.pos && ship.pos.row !== undefined && ship.pos.col !== undefined &&
                     !(ship.pos.row === -1 && ship.pos.col === -1);
    if (isPlaced) {
        card.classList.add("placed");
    }

    // Mark as selected during setup
    if (isYours && selectedVehicle && selectedVehicle.type === 'ship' && selectedVehicle.id === ship.id) {
        card.classList.add("selected");
    }

    // Make clickable during setup for unplaced ships
    if (isYours && lastPhase === "setup" && !isPlaced) {
        card.classList.add("selectable");
        card.addEventListener("click", () => selectVehicle('ship', ship.id));
    }

    // Ship name
    const nameEl = document.createElement("div");
    nameEl.className = "ship-name";
    nameEl.textContent = ship.name;
    card.appendChild(nameEl);

    // Ship form (mini grid visualization)
    const formGrid = createShipFormGrid(ship.coords);
    card.appendChild(formGrid);

    // Abilities
    if (ship.abilities && ship.abilities.length > 0) {
        const abilitiesContainer = document.createElement("div");
        abilitiesContainer.className = "ship-abilities";

        for (const ability of ship.abilities) {
            const abilityEl = createAbilityButton(ability, ship.id, isYours);
            abilitiesContainer.appendChild(abilityEl);
        }

        card.appendChild(abilitiesContainer);
    }

    return card;
}

// Plane colors array - matches CSS plane-color-N classes
const PLANE_COLORS = ['#f59e0b', '#ec4899', '#8b5cf6', '#22c55e', '#06b6d4', '#ef4444'];

function createPlaneCard(plane, isYours, planeIndex = 0) {
    const card = document.createElement("div");
    card.className = "plane-card";
    card.dataset.planeId = plane.id;

    if (plane.isdestroyed) {
        card.classList.add("destroyed");
    }

    // Mark as placed if plane has valid position
    const isPlaced = plane.pos && plane.pos.row !== undefined && plane.pos.col !== undefined &&
                     !(plane.pos.row === -1 && plane.pos.col === -1);
    if (isPlaced) {
        card.classList.add("placed");
    }

    // Mark as selected during setup
    if (isYours && selectedVehicle && selectedVehicle.type === 'plane' && selectedVehicle.id === plane.id) {
        card.classList.add("selected");
    }

    // Make clickable during setup for unplaced planes
    if (isYours && lastPhase === "setup" && !isPlaced) {
        card.classList.add("selectable");
        card.addEventListener("click", () => selectVehicle('plane', plane.id));
    }

    // Plane name
    const nameEl = document.createElement("div");
    nameEl.className = "plane-name";
    nameEl.textContent = plane.name;
    card.appendChild(nameEl);

    // Plane icon/indicator with color
    const planeColor = PLANE_COLORS[planeIndex % PLANE_COLORS.length];
    const iconEl = document.createElement("div");
    iconEl.className = "plane-icon";
    iconEl.textContent = "\u2708";
    iconEl.style.color = planeColor;
    card.appendChild(iconEl);

    // Position indicator (if placed)
    if (isPlaced) {
        const posEl = document.createElement("div");
        posEl.className = "plane-position";
        const rowLabel = String.fromCharCode(65 + plane.pos.row);
        posEl.textContent = `${rowLabel}${plane.pos.col + 1}`;
        card.appendChild(posEl);
    }

    // Abilities
    if (plane.abilities && plane.abilities.length > 0) {
        const abilitiesContainer = document.createElement("div");
        abilitiesContainer.className = "plane-abilities";

        for (const ability of plane.abilities) {
            const abilityEl = createAbilityButton(ability, plane.id, isYours);
            abilitiesContainer.appendChild(abilityEl);
        }

        card.appendChild(abilitiesContainer);
    }

    return card;
}

function renderFleetPanel(container, ships, planes, isYours) {
    container.innerHTML = "";

    const hasShips = ships && ships.length > 0;
    const hasPlanes = planes && planes.length > 0;

    if (!hasShips && !hasPlanes) {
        const emptyMsg = document.createElement("div");
        emptyMsg.className = "fleet-empty";
        emptyMsg.textContent = "No vehicles";
        container.appendChild(emptyMsg);
        return;
    }

    // Render ships
    if (hasShips) {
        for (const ship of ships) {
            const card = createShipCard(ship, isYours);
            container.appendChild(card);
        }
    }

    // Render planes with color index
    if (hasPlanes) {
        planes.forEach((plane, planeIndex) => {
            const card = createPlaneCard(plane, isYours, planeIndex);
            container.appendChild(card);
        });
    }
}

function updateFleetPanels(fleetView) {
    if (!fleetView) return;

    // Render your fleet (ships and planes)
    renderFleetPanel(
        yourFleetList, 
        fleetView.yourships || [], 
        fleetView.yourplanes || [],
        true
    );

    // Render opponent fleet (ships and planes)
    renderFleetPanel(
        opponentFleetList, 
        fleetView.opponentships || [], 
        fleetView.opponentplanes || [],
        false
    );
}

function handleAbilityClick(vehicleId, abilityType) {
    const config = ABILITY_CONFIG[abilityType];
    if (!config) {
        showMessage(`Unknown ability type: ${abilityType}`, "error");
        return;
    }

    // If ability needs a firing pattern, prompt user to select one first
    if (config.needsFiringPattern) {
        showFiringPatternSelector(vehicleId, abilityType, config.patterns);
        return;
    }

    // Otherwise, activate ability mode directly
    activateAbilityMode(vehicleId, abilityType, null);
}

function showFiringPatternSelector(vehicleId, abilityType, patterns) {
    // Create a simple pattern selector UI
    const selector = document.createElement("div");
    selector.className = "pattern-selector-overlay";
    selector.id = "patternSelector";

    const content = document.createElement("div");
    content.className = "pattern-selector-content";

    const title = document.createElement("h3");
    title.textContent = `Select ${getAbilityDisplayName(abilityType)} Pattern`;
    content.appendChild(title);

    const buttonsDiv = document.createElement("div");
    buttonsDiv.className = "pattern-buttons";

    for (const pattern of patterns) {
        const btn = document.createElement("button");
        btn.className = "btn-secondary pattern-btn";
        btn.textContent = pattern.charAt(0).toUpperCase() + pattern.slice(1);
        btn.addEventListener("click", () => {
            document.getElementById("patternSelector")?.remove();
            activateAbilityMode(vehicleId, abilityType, pattern);
        });
        buttonsDiv.appendChild(btn);
    }

    const cancelBtn = document.createElement("button");
    cancelBtn.className = "btn-secondary";
    cancelBtn.textContent = "Cancel";
    cancelBtn.addEventListener("click", () => {
        document.getElementById("patternSelector")?.remove();
    });
    buttonsDiv.appendChild(cancelBtn);

    content.appendChild(buttonsDiv);
    selector.appendChild(content);
    document.body.appendChild(selector);
}

function activateAbilityMode(vehicleId, abilityType, firingPattern) {
    activeAbility = { vehicleId, type: abilityType, firingPattern };
    selectedVehicle = null; // Clear placement selection

    const config = ABILITY_CONFIG[abilityType];
    const targetGrid = config.targetType === "opponent" ? "opponent's grid" : "your grid";
    showMessage(`${getAbilityDisplayName(abilityType)} active - Click on ${targetGrid} to target`, "info");

    // Add visual indicator to the grids
    updateAbilityModeUI();
}

function cancelAbilityMode() {
    activeAbility = null;
    updateAbilityModeUI();
}

function updateAbilityModeUI() {
    // Add/remove ability-mode class from grids
    const ownGridWrapper = ownGrid.closest(".grid-wrapper");
    const oppGridWrapper = oppGrid.closest(".grid-wrapper");

    if (activeAbility) {
        const config = ABILITY_CONFIG[activeAbility.type];
        if (config.targetType === "opponent") {
            oppGridWrapper?.classList.add("ability-target");
            ownGridWrapper?.classList.remove("ability-target");
        } else {
            ownGridWrapper?.classList.add("ability-target");
            oppGridWrapper?.classList.remove("ability-target");
        }
    } else {
        ownGridWrapper?.classList.remove("ability-target");
        oppGridWrapper?.classList.remove("ability-target");
    }
}

function executeAbility(row, col) {
    if (!activeAbility || !lastSetupInfo) return;

    const { vehicleId, type, firingPattern } = activeAbility;
    const config = ABILITY_CONFIG[type];

    let abilityData;
    switch (type) {
        case "torpedo":
            abilityData = {
                firingpattern: firingPattern,
                startpoint: { row, col }
            };
            break;
        case "exocet":
            abilityData = { target: { row, col } };
            break;
        case "apache":
            abilityData = {
                firingpattern: firingPattern,
                target: { row, col }
            };
            break;
        case "tomahawk":
            abilityData = {
                firingpattern: firingPattern,
                target: { row, col }
            };
            break;
        case "scan":
            abilityData = { target: { row, col } };
            break;
        case "reveal":
            abilityData = {
                firingpattern: firingPattern,
                target: { row, col }
            };
            break;
        case "relocate":
            // For relocate, we need to specify which ship to move
            // For now, we'll need another selection step - this is complex
            // Simplified: prompt for ship selection or use the activating vehicle's ship
            abilityData = {
                shipid: vehicleId, // Move the ship that has the ability
                target: { row, col }
            };
            break;
        default:
            showMessage(`Unknown ability: ${type}`, "error");
            cancelAbilityMode();
            return;
    }

    const message = {
        gameid: lastSetupInfo.gameid,
        userid: lastSetupInfo.you,
        sessionaction: {
            type: "activateability",
            data: {
                vehicleid: vehicleId,
                abilityaction: {
                    type: type,
                    data: abilityData
                }
            }
        }
    };

    logLine(`Sending activateability: ${type} at (${row}, ${col})`);
    socket.send(JSON.stringify(message));

    // Clear ability mode after execution
    cancelAbilityMode();
}

function selectVehicle(type, id) {
    selectedVehicle = { type, id };
    // Re-render fleet panels to show selection
    if (lastSetupInfo && lastSetupInfo.fleetview) {
        updateFleetPanels(lastSetupInfo.fleetview);
    }
    showMessage(`Selected ${type} for placement`, "info");
}

function selectNextUnplacedVehicle(fleetView) {
    // First check for unplaced ships
    for (const ship of fleetView.yourships || []) {
        const isPlaced = ship.pos && ship.pos.row !== undefined && ship.pos.col !== undefined &&
                         !(ship.pos.row === -1 && ship.pos.col === -1);
        if (!isPlaced && !placedShipIds.has(ship.id)) {
            selectVehicle('ship', ship.id);
            return;
        }
    }
    // Then check for unplaced planes
    for (const plane of fleetView.yourplanes || []) {
        const isPlaced = plane.pos && plane.pos.row !== undefined && plane.pos.col !== undefined &&
                         !(plane.pos.row === -1 && plane.pos.col === -1);
        if (!isPlaced && !placedPlaneIds.has(plane.id)) {
            selectVehicle('plane', plane.id);
            return;
        }
    }
    // All placed
    selectedVehicle = null;
}

// === Click Handlers ===
function handleOwnGridClick(row, col) {
    // Check if we're in ability mode targeting own grid (e.g., relocate)
    if (activeAbility) {
        const config = ABILITY_CONFIG[activeAbility.type];
        if (config.targetType === "own") {
            executeAbility(row, col);
            return;
        }
    }

    // Ship/plane placement during setup
    if (!selectedVehicle || !lastSetupInfo) return;

    if (selectedVehicle.type === 'ship') {
        const message = {
            gameid: lastSetupInfo.gameid,
            userid: lastSetupInfo.you,
            sessionaction: {
                type: "placeship",
                data: {
                    position: { row, col },
                    rotation: rotation,
                    shipid: selectedVehicle.id,
                },
            },
        };
        sendMessage(message);
    } else if (selectedVehicle.type === 'plane') {
        const message = {
            gameid: lastSetupInfo.gameid,
            userid: lastSetupInfo.you,
            sessionaction: {
                type: "placeplane",
                data: {
                    position: { row, col },
                    planeid: selectedVehicle.id,
                },
            },
        };
        sendMessage(message);
    }
}

function handleOwnGridHover(row, col) {
    hoveredCell = { row, col };

    // Only show preview during setup phase
    if (!selectedVehicle || !lastSetupInfo) return;
    if (lastPhase !== "setup") return;

    if (selectedVehicle.type === 'ship') {
        const message = {
            gameid: lastSetupInfo.gameid,
            userid: lastSetupInfo.you,
            sessionaction: {
                type: "checkplacement",
                data: {
                    position: { row, col },
                    rotation: rotation,
                    shipid: selectedVehicle.id,
                },
            },
        };
        sendMessage(message);
    } else if (selectedVehicle.type === 'plane') {
        const message = {
            gameid: lastSetupInfo.gameid,
            userid: lastSetupInfo.you,
            sessionaction: {
                type: "checkplaneplacement",
                data: {
                    position: { row, col },
                    planeid: selectedVehicle.id,
                },
            },
        };
        sendMessage(message);
    }
}

function clearPreview() {
    // Clear preview overlay classes from all cells
    for (const cell of ownGrid.children) {
        cell.classList.remove("preview-valid", "preview-invalid");
    }
}

function clearPreviewAndHover() {
    hoveredCell = null;
    clearPreview();
}

function handleOppGridClick(row, col) {
    if (!lastSetupInfo) return;

    // Check if we're in ability mode targeting opponent grid
    if (activeAbility) {
        const config = ABILITY_CONFIG[activeAbility.type];
        if (config.targetType === "opponent") {
            executeAbility(row, col);
            return;
        }
    }

    // Default: regular fire action
    const message = {
        gameid: lastSetupInfo.gameid,
        userid: lastSetupInfo.you,
        sessionaction: {
            type: "fire",
            data: {
                target: { row, col },
            },
        },
    };

    sendMessage(message);
}

function sendMessage(msg) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(JSON.stringify(msg));
        logLine("Sent: " + JSON.stringify(msg));
    }
}

// === Event Handlers ===
rotateBtn.addEventListener("click", () => {
    rotation = (rotation + 1) % 4;
    const directions = ["Right", "Up", "Left", "Down"];
    showMessage(`Rotation: ${directions[rotation]}`, "info");
    
    // Refresh preview if hovering over a cell
    if (hoveredCell) {
        handleOwnGridHover(hoveredCell.row, hoveredCell.col);
    }
});

// Keyboard shortcut for rotation
document.addEventListener("keydown", (e) => {
    if (e.key === "r" || e.key === "R") {
        // Don't trigger if user is typing in an input
        if (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA") return;
        if (lastPhase !== "setup") return;

        rotateBtn.click();
    }

    // Cancel ability mode with Escape
    if (e.key === "Escape") {
        if (activeAbility) {
            cancelAbilityMode();
            showMessage("Ability cancelled", "info");
        }
        // Also close pattern selector if open
        document.getElementById("patternSelector")?.remove();
    }
});

readyBtn.addEventListener("click", () => {
    if (!lastSetupInfo) return;

    const message = {
        gameid: lastSetupInfo.gameid,
        userid: lastSetupInfo.you,
        sessionaction: {
            type: "ready",
            data: null,
        },
    };

    sendMessage(message);
});

connectBtn.addEventListener("click", () => {
    const userId = userIdInput.value.trim();
    const gameIdValue = gameIdInput.value.trim();

    if (!userId || !gameIdValue) {
        showMessage("Please enter both User ID and Game ID", "error");
        return;
    }

    // Store globally
    myUserId = userId;
    gameId = gameIdValue;

    setConnectionStatus("connecting");

    const WS_URL = import.meta.env.VITE_WS_URL;
    if (!WS_URL) {
        showMessage("Missing VITE_WS_URL configuration", "error");
        throw new Error("Missing VITE_WS_URL. Create webapp/.env.local (see .env.example).");
    }

    socket = new WebSocket(WS_URL);

    socket.onopen = () => {
        setConnectionStatus("connected");
        logLine("WebSocket connected");

        const helloMessage = {
            type: "hello",
            userid: myUserId,
            gameid: gameId,
        };

        sendMessage(helloMessage);
    };

    socket.onmessage = (event) => {
        logLine("Received: " + event.data);

        let obj;
        try {
            obj = JSON.parse(event.data);
        } catch {
            return;
        }

        for (const key in obj) {
            switch (key) {
                case "setupinfo":
                    applySetupInfo(obj[key]);
                    break;
                case "snapshot":
                    applySnapshot(obj[key]);
                    break;
                case "actionresult":
                    applyActionResult(obj[key]);
                    break;
                case "waiting":
                    showMessage("Waiting for opponent to join...", "info");
                    break;
                case "rematchrequest":
                    handleRematchRequest(obj[key]);
                    break;
                case "rematchstart":
                    handleRematchStart();
                    break;
                case "error":
                    showMessage(`Error: ${obj[key]}`, "error");
                    break;
            }
        }
    };

    socket.onerror = () => {
        setConnectionStatus("disconnected");
        showMessage("Connection error", "error");
        logLine("WebSocket error");
    };

    socket.onclose = () => {
        setConnectionStatus("disconnected");
        logLine("WebSocket closed");
    };
});

// === Rematch and Home Button Handlers ===
rematchBtn.addEventListener("click", () => {
    if (!myUserId || !gameId) return;
    
    rematchRequested = true;
    updateRematchButton();
    
    const message = {
        gameid: gameId,
        userid: myUserId,
        sessionaction: {
            type: "rematch"
        }
    };
    
    sendMessage(message);
    showMessage("Rematch requested", "info");
});

homeBtn.addEventListener("click", () => {
    // Reload the page to go back to connection screen
    window.location.reload();
});

function handleRematchRequest(data) {
    opponentWantsRematch = true;
    updateRematchButton();
    showMessage("Opponent wants a rematch!", "info");
}

function handleRematchStart() {
    // Reset game state
    rematchRequested = false;
    opponentWantsRematch = false;
    hideGameOver();
    updateRematchButton();
    placedShipIds.clear();
    placedPlaneIds.clear();
    selectedVehicle = null;
    rotation = 0;
    lastPhase = null;

    // Clear grids
    ownGrid.innerHTML = "";
    oppGrid.innerHTML = "";

    // Clear fleet panels
    yourFleetList.innerHTML = "";
    opponentFleetList.innerHTML = "";

    showMessage("Rematch starting!", "success");
}

// === Apply Server Data ===
function applySetupInfo(setupInfo) {
    lastSetupInfo = setupInfo;
    placedShipIds.clear();
    lastPhase = setupInfo.phase || "setup";

    // Switch views
    connectSection.classList.add("hidden");
    gameSection.classList.remove("hidden");

    // Populate info
    meSpan.textContent = setupInfo.you || "—";
    opponentSpan.textContent = setupInfo.opponent || "—";
    updatePhaseDisplay(lastPhase);

    // Build grids
    buildGrids(setupInfo.boardrows, setupInfo.boardcols);

    // Render initial fleet panels from setupinfo
    if (setupInfo.fleetview) {
        updateFleetPanels(setupInfo.fleetview);
        // Auto-select first unplaced vehicle
        selectNextUnplacedVehicle(setupInfo.fleetview);
    }

    showMessage("Game started! Click a ship to select it, then click on your grid to place it.", "success");
}

function applySnapshot(snapshot) {
    if (!snapshot) return;

    const myUserId = lastSetupInfo?.you;
    const currentPhase = snapshot.phase;
    
    // Check for game end BEFORE updating lastPhase
    if (currentPhase === "finished" && lastPhase !== "finished") {
        // Determine if we won by checking if we have any unhit ships left
        const ownGridData = snapshot.userview?.boardview?.owngrid || [];
        const hasShipsLeft = ownGridData.some(entry => entry.state === "ship");
        showGameOver(hasShipsLeft);
    }

    // Now update the phase tracking
    lastPhase = currentPhase;

    updatePhaseDisplay(currentPhase);
    updateTurnIndicator(snapshot.currentturn, myUserId);
    updateReadyStatus(snapshot.youready, snapshot.opponentready);

    // Clear all state classes from grids (but not preview classes)
    const stateClasses = [
        "ship", "hit", "miss", 
        "revealedmiss", "revealedhit", "scannedpositive",
        "plane", "plane-color-0", "plane-color-1", "plane-color-2", 
        "plane-color-3", "plane-color-4", "plane-color-5"
    ];
    for (const cell of ownGrid.children) {
        cell.classList.remove(...stateClasses);
    }
    for (const cell of oppGrid.children) {
        cell.classList.remove(...stateClasses);
    }

    const cols = lastSetupInfo?.boardcols || 10;

    // Apply own grid state
    const ownGridData = snapshot.userview?.boardview?.owngrid || [];
    for (const entry of ownGridData) {
        const r = entry.coord?.row;
        const c = entry.coord?.col;
        const state = entry.state;

        if (typeof r !== "number" || typeof c !== "number") continue;

        const idx = r * cols + c;
        const cell = ownGrid.children[idx];
        if (cell && state) cell.classList.add(state);
    }

    // Apply planes on own grid (overlay on ship cells)
    const yourPlanes = snapshot.fleetview?.yourplanes || [];
    yourPlanes.forEach((plane, planeIndex) => {
        if (!plane.pos || plane.pos.row === -1 || plane.isdestroyed) return;

        const r = plane.pos.row;
        const c = plane.pos.col;

        if (typeof r !== "number" || typeof c !== "number") return;

        const idx = r * cols + c;
        const cell = ownGrid.children[idx];
        if (cell) {
            cell.classList.add("plane");
            cell.classList.add(`plane-color-${planeIndex % 6}`);
        }
    });

    // Apply opponent grid state
    const oppGridData = snapshot.userview?.boardview?.opponentgrid || [];
    for (const entry of oppGridData) {
        const r = entry.coord?.row;
        const c = entry.coord?.col;
        const state = entry.state;

        if (typeof r !== "number" || typeof c !== "number") continue;

        const idx = r * cols + c;
        const cell = oppGrid.children[idx];
        if (cell && state) cell.classList.add(state);
    }

    // Update fleet panels from snapshot
    if (snapshot.fleetview) {
        updateFleetPanels(snapshot.fleetview);
    }
}

function applyActionResult(result) {
    switch (result.type) {
        case "fireresult":
            applyFireResult(result);
            break;
        case "placeshipresult":
            applyPlaceShipResult(result);
            break;
        case "placeplaneresult":
            applyPlacePlaneResult(result);
            break;
        case "readyresult":
            applyReadyResult(result);
            break;
        case "transientoverlayresult":
            applyTransientOverlayResult(result);
            break;
        case "activateabilityresult":
            applyActivateAbilityResult(result);
            break;
    }
}

function applyFireResult(result) {
    if (result.success) {
        // Compare actinguser to determine if we fired or were fired upon
        const iDidThis = result.actinguser === myUserId;

        if (result.data?.ishit) {
            if (result.data.issunk) {
                if (iDidThis) {
                    showMessage(`You sank their ${result.data.sunkname}!`, "success");
                } else {
                    showMessage(`Your ${result.data.sunkname} was sunk!`, "error");
                }
                // Fleet panels will be updated by the snapshot that follows
            } else {
                if (iDidThis) {
                    showMessage("Hit!", "success");
                } else {
                    showMessage("Your ship was hit!", "error");
                }
            }
        } else {
            // Miss - only show for the shooter
            if (iDidThis) {
                showMessage("Miss", "info");
            }
        }
    } else {
        const errorMessages = {
            notyourturn: "It's not your turn",
            invalidplacement: "You can't fire there",
        };
        showMessage(errorMessages[result.error] || "Unable to fire", "error");
    }
}

function applyPlaceShipResult(result) {
    if (result.success) {
        // Mark this ship as placed and select next unplaced vehicle
        if (selectedVehicle && selectedVehicle.type === 'ship') {
            placedShipIds.add(selectedVehicle.id);
        }
        if (lastSetupInfo && lastSetupInfo.fleetview) {
            selectNextUnplacedVehicle(lastSetupInfo.fleetview);
        }
    } else {
        const errorMessages = {
            wrongphase: "Ships can only be placed during setup",
            invalidplacement: "Invalid placement - ships cannot overlap or go out of bounds",
            shipnotfound: "Ship not found",
        };
        showMessage(errorMessages[result.error] || "Unable to place ship", "error");
    }
}

function applyPlacePlaneResult(result) {
    if (result.success) {
        // Mark this plane as placed and select next unplaced vehicle
        if (selectedVehicle && selectedVehicle.type === 'plane') {
            placedPlaneIds.add(selectedVehicle.id);
        }
        if (lastSetupInfo && lastSetupInfo.fleetview) {
            selectNextUnplacedVehicle(lastSetupInfo.fleetview);
        }
    } else {
        const errorMessages = {
            wrongphase: "Planes can only be placed during setup",
            invalidplacement: "Invalid placement - planes must be placed on a carrier",
            vehiclenotfound: "Plane not found",
        };
        showMessage(errorMessages[result.error] || "Unable to place plane", "error");
    }
}

function applyReadyResult(result) {
    if (result.success) {
        // Don't show "waiting" message here - let the snapshot handle the phase transition
        // The snapshot will tell us if both players are ready
    } else {
        const errorMessages = {
            invalidplacement: "Place all ships before readying up",
            notyourturn: "Your fleet placement is invalid",
        };
        showMessage(errorMessages[result.error] || "Unable to ready up", "error");
    }
}

function applyTransientOverlayResult(result) {
    if (!result.success) return;

    const data = result.data;
    if (!data || !data.overlay) return;

    const cols = lastSetupInfo?.boardcols || 10;

    // Clear any existing preview
    clearPreview();

    // Process the overlay map
    for (const coordKey in data.overlay) {
        // Parse coordinate key "row,col"
        const [rowStr, colStr] = coordKey.split(",");
        const r = parseInt(rowStr, 10);
        const c = parseInt(colStr, 10);

        if (typeof r !== "number" || typeof c !== "number" || isNaN(r) || isNaN(c)) continue;

        const states = data.overlay[coordKey];
        if (!Array.isArray(states) || states.length === 0) continue;

        // Only show preview for in-bounds cells
        if (r < 0 || c < 0) continue;

        const idx = r * cols + c;
        const cell = ownGrid.children[idx];
        if (!cell) continue;

        // Apply CSS classes based on the states
        // Priority: invalidPlacement > validPlacement > targetedSquare
        if (states.includes("invalidplacement")) {
            cell.classList.add("preview-invalid");
        } else if (states.includes("validplacement")) {
            cell.classList.add("preview-valid");
        } else if (states.includes("targetedsquare")) {
            cell.classList.add("preview-valid");
        }
    }
}

function applyActivateAbilityResult(result) {
    const iDidThis = result.actinguser === myUserId;

    if (result.success) {
        const data = result.data;

        // Handle different ability result types
        if (data?.ishit !== undefined) {
            // Torpedo or bulk fire result
            if (data.ishit) {
                if (iDidThis) {
                    showMessage("Ability hit!", "success");
                }
            } else {
                if (iDidThis) {
                    showMessage("Ability missed", "info");
                }
            }
        } else if (data?.isfound !== undefined) {
            // Scan result
            if (iDidThis) {
                if (data.isfound) {
                    showMessage("Scan detected enemy ships in the area!", "success");
                } else {
                    showMessage("Scan found no ships in the area", "info");
                }
            }
        } else if (data?.hitsrevealed !== undefined) {
            // Reveal result
            if (iDidThis) {
                const count = data.hitsrevealed?.length || 0;
                if (count > 0) {
                    showMessage(`Revealed ${count} ship positions!`, "success");
                } else {
                    showMessage("No ships found in revealed area", "info");
                }
            }
        } else {
            // Generic success (e.g., relocate)
            if (iDidThis) {
                showMessage("Ability activated successfully", "success");
            }
        }
    } else {
        const errorMessages = {
            notyourturn: "It's not your turn",
            notyourship: "That's not your ship",
            shipsunk: "That ship is sunk and cannot use abilities",
            nosuchability: "That vehicle doesn't have this ability",
            outofbounds: "Target is out of bounds",
        };
        showMessage(errorMessages[result.error] || "Unable to activate ability", "error");
    }
}
