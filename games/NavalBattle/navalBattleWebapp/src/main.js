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
const shipSelect = document.getElementById("shipSelect");
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
let hoveredCell = null; // Track currently hovered cell for preview refresh
let myUserId = null;
let gameId = null;
let rematchRequested = false;
let opponentWantsRematch = false;

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
        torpedo: "??",
        exocet: "??",
        apache: "??",
        tomahawk: "??",
        scan: "??",
        reveal: "???",
        relocate: "??"
    };
    return icons[abilityType] || "?";
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
    element.dataset.shipId = shipId;

    const icon = document.createElement("span");
    icon.className = "ability-icon";
    icon.textContent = getAbilityIcon(ability.type);

    const name = document.createElement("span");
    name.className = "ability-name";
    name.textContent = getAbilityDisplayName(ability.type);

    const uses = document.createElement("span");
    uses.className = "ability-uses";
    if (ability.usagepolicy === "unlimited") {
        uses.textContent = "?";
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

function createPlaneCard(plane, isYours) {
    const card = document.createElement("div");
    card.className = "plane-card";
    card.dataset.planeId = plane.id;

    if (plane.isdestroyed) {
        card.classList.add("destroyed");
    }

    // Plane name
    const nameEl = document.createElement("div");
    nameEl.className = "plane-name";
    nameEl.textContent = plane.name;
    card.appendChild(nameEl);

    // Plane icon/indicator
    const iconEl = document.createElement("div");
    iconEl.className = "plane-icon";
    iconEl.textContent = "?";
    card.appendChild(iconEl);

    // Position indicator (if placed)
    if (plane.pos && plane.pos.row !== undefined && plane.pos.col !== undefined) {
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

    // Render planes
    if (hasPlanes) {
        for (const plane of planes) {
            const card = createPlaneCard(plane, isYours);
            container.appendChild(card);
        }
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

function handleAbilityClick(shipId, abilityType) {
    // For now, just log the click - actual functionality to be implemented
    logLine(`Ability clicked: Ship ${shipId}, Ability: ${abilityType}`);
    showMessage(`Selected ${getAbilityDisplayName(abilityType)} ability`, "info");
}

// === Click Handlers ===
function handleOwnGridClick(row, col) {
    const shipIdStr = shipSelect.value;
    if (!shipIdStr || !lastSetupInfo) return;

    const message = {
        gameid: lastSetupInfo.gameid,
        userid: lastSetupInfo.you,
        sessionaction: {
            type: "placeship",
            data: {
                position: { row, col },
                rotation: rotation,
                shipid: Number(shipIdStr),
            },
        },
    };

    sendMessage(message);
}

function handleOwnGridHover(row, col) {
    hoveredCell = { row, col };
    
    const shipIdStr = shipSelect.value;
    if (!shipIdStr || !lastSetupInfo) return;
    
    // Only show preview during setup phase
    if (lastPhase !== "setup") return;

    const message = {
        gameid: lastSetupInfo.gameid,
        userid: lastSetupInfo.you,
        sessionaction: {
            type: "checkplacement",
            data: {
                position: { row, col },
                rotation: rotation,
                shipid: Number(shipIdStr),
            },
        },
    };

    sendMessage(message);
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

function selectNextShip(currentShipId) {
    placedShipIds.add(currentShipId);
    
    // Find next unplaced ship
    const options = Array.from(shipSelect.options);
    for (const opt of options) {
        const id = Number(opt.value);
        if (!placedShipIds.has(id)) {
            shipSelect.value = opt.value;
            return;
        }
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

    // Populate ship selector from fleetview
    shipSelect.innerHTML = "";
    for (const ship of setupInfo.fleetview?.yourships || []) {
        const opt = document.createElement("option");
        opt.value = String(ship.id);
        opt.textContent = ship.name;
        shipSelect.appendChild(opt);
    }

    // Build grids
    buildGrids(setupInfo.boardrows, setupInfo.boardcols);

    // Render initial fleet panels from setupinfo
    if (setupInfo.fleetview) {
        updateFleetPanels(setupInfo.fleetview);
    }

    showMessage("Game started! Place your ships.", "success");
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
    for (const cell of ownGrid.children) {
        cell.classList.remove("ship", "hit", "miss");
    }
    for (const cell of oppGrid.children) {
        cell.classList.remove("ship", "hit", "miss");
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
        case "readyresult":
            applyReadyResult(result);
            break;
        case "checkplacementresult":
            applyCheckPlacementResult(result);
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
        // Select the next ship automatically
        const currentShipId = Number(shipSelect.value);
        selectNextShip(currentShipId);
    } else {
        const errorMessages = {
            wrongphase: "Ships can only be placed during setup",
            invalidplacement: "Invalid placement - ships cannot overlap or go out of bounds",
            shipnotfound: "Ship not found",
        };
        showMessage(errorMessages[result.error] || "Unable to place ship", "error");
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

function applyCheckPlacementResult(result) {
    if (!result.success) return;
    
    const data = result.data;
    if (!data) return;
    
    const cols = lastSetupInfo?.boardcols || 10;
    const previewClass = data.valid ? "preview-valid" : "preview-invalid";
    
    // Clear any existing preview
    clearPreview();
    
    // Apply preview overlay to the specified coords
    for (const coord of data.coords || []) {
        const r = coord.row;
        const c = coord.col;
        
        if (typeof r !== "number" || typeof c !== "number") continue;
        
        // Only show preview for in-bounds cells
        if (r < 0 || c < 0) continue;
        
        const idx = r * cols + c;
        const cell = ownGrid.children[idx];
        if (cell) {
            cell.classList.add(previewClass);
        }
    }
}
