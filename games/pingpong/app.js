const $ = (selector) => document.querySelector(selector);
const video = $("#video");
const canvas = $("#view");
const ctx = canvas.getContext("2d");
const analysisCanvas = document.createElement("canvas");
const analysis = analysisCanvas.getContext("2d", { willReadFrequently: true });

const DEFAULT_TABLE_CORNERS = [
  { x: .39, y: .34 },
  { x: .62, y: .34 },
  { x: .69, y: .74 },
  { x: .30, y: .74 }
];

function validTableCorners(corners) {
  return Array.isArray(corners)
    && corners.length === 4
    && corners.every((point) => Number.isFinite(point?.x)
      && Number.isFinite(point?.y)
      && point.x >= 0 && point.x <= 1
      && point.y >= 0 && point.y <= 1);
}

function loadSavedTableCorners() {
  try {
    const saved = JSON.parse(localStorage.getItem("pingpong-table-corners-v3"));
    if (validTableCorners(saved)) return saved;
  } catch (error) {
    console.debug("Saved table region unavailable", error);
  }
  return DEFAULT_TABLE_CORNERS.map((point) => ({ ...point }));
}

const state = {
  score: { left: 0, right: 0 },
  names: { left: "Player 1", right: "Player 2" },
  server: "left",
  firstServer: "left",
  history: [],
  auto: true,
  calibrating: false,
  tableCorners: loadSavedTableCorners(),
  calibrationPoints: [],
  previous: null,
  lastBall: null,
  detections: [],
  rallyActive: false,
  lastDetectionAt: 0,
  lastAwardAt: 0,
  stream: null,
  animationId: null,
  matchRunning: false,
  audioContext: null,
  audioSource: null,
  mediaElementSource: null,
  analyser: null,
  audioData: null,
  audioEnabled: false,
  noiseFloor: .012,
  lastHitAt: 0,
  hitTimes: [],
  hitCount: 0,
  fileMode: false,
  facingMode: "environment",
  mediaRecorder: null,
  recordedChunks: [],
  recordingUrl: null,
  recordingStartedAt: 0,
  recordingTimer: null,
  recording: false,
  wakeLock: null,
  settings: {
    micSensitivity: 1.0,
    ballColor: "white", // or "orange"
  }
};

analysisCanvas.width = 320;
analysisCanvas.height = 180;

function renderScore() {
  $("#leftScore").textContent = state.score.left;
  $("#rightScore").textContent = state.score.right;
  $("#leftName").textContent = state.names.left;
  $("#rightName").textContent = state.names.right;
  $("#leftServe").classList.toggle("hidden", state.server !== "left");
  $("#rightServe").classList.toggle("hidden", state.server !== "right");
  const winner = getWinner();
  $("#gameState").textContent = winner ? `${state.names[winner]} wins` : "Game 1";
  renderHistory();
}

function getWinner() {
  const { left, right } = state.score;
  if (Math.max(left, right) >= 11 && Math.abs(left - right) >= 2) return left > right ? "left" : "right";
  return null;
}

function updateServer() {
  const total = state.score.left + state.score.right;
  const deuce = state.score.left >= 10 && state.score.right >= 10;
  const turns = deuce ? total - 20 : Math.floor(total / 2);
  state.server = turns % 2 === 0 ? state.firstServer : opposite(state.firstServer);
}

function award(side, source = "Manual") {
  if (getWinner()) return;
  state.history.push({
    side,
    source,
    before: { ...state.score },
    at: new Date()
  });
  state.score[side] += 1;
  updateServer();
  renderScore();
  showToast(`${state.names[side]} +1 · ${source}`);
}

function undo() {
  const event = state.history.pop();
  if (!event) return;
  state.score = event.before;
  updateServer();
  renderScore();
  showToast("Last point undone");
}

function reset() {
  if ((state.score.left || state.score.right) && !confirm("Reset the current game?")) return;
  state.score = { left: 0, right: 0 };
  state.history = [];
  state.server = state.firstServer;
  renderScore();
}

function opposite(side) { return side === "left" ? "right" : "left"; }

function renderHistory() {
  const list = $("#history");
  list.replaceChildren();

  if (!state.history.length) {
    const emptyItem = document.createElement("li");
    emptyItem.className = "empty-history";
    emptyItem.textContent = "No points yet";
    list.append(emptyItem);
    return;
  }

  state.history.slice().reverse().forEach((event) => {
    const item = document.createElement("li");
    const summary = document.createElement("span");
    const time = document.createElement("time");

    summary.textContent = `${state.names[event.side]} +1 · ${event.source}`;
    time.textContent = event.at.toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    });

    item.append(summary, time);
    list.append(item);
  });
}

let toastTimer;
function showToast(message) {
  const toast = $("#toast");
  toast.textContent = message;
  toast.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove("show"), 2300);
}

async function requestWakeLock() {
  if (!("wakeLock" in navigator) || state.wakeLock) return;
  try {
    state.wakeLock = await navigator.wakeLock.request("screen");
    state.wakeLock.addEventListener("release", () => {
      state.wakeLock = null;
    }, { once: true });
  } catch (error) {
    console.debug("Screen wake lock unavailable", error);
  }
}

function releaseWakeLock() {
  state.wakeLock?.release().catch(() => {});
  state.wakeLock = null;
}

async function startCamera() {
  stopSource();
  state.fileMode = false;
  video.muted = true;
  const videoConstraints = {
    facingMode: { ideal: state.facingMode },
    width: { ideal: 1920 },
    height: { ideal: 1080 },
    aspectRatio: { ideal: 16 / 9 },
    frameRate: { ideal: 30 }
  };
  try {
    try {
      state.stream = await navigator.mediaDevices.getUserMedia({
        video: videoConstraints,
        audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false }
      });
    } catch (audioError) {
      state.stream = await navigator.mediaDevices.getUserMedia({
        video: videoConstraints,
        audio: false
      });
      showToast("Microphone unavailable. Manual scoring only.");
    }
    video.srcObject = state.stream;
    await video.play();
    await setupAudio(state.stream);
    sourceReady("Camera ready · Tap Start Match");
    return true;
  } catch (error) {
    showToast("Camera unavailable. Check browser permissions.");
    console.error(error);
    return false;
  }
}

async function setupAudio(stream) {
  const audioTrack = stream.getAudioTracks()[0];
  if (!audioTrack) {
    state.audioEnabled = false;
    $("#audioStatus").textContent = "Manual mode";
    return;
  }
  const AudioContext = window.AudioContext || window.webkitAudioContext;
  if (!AudioContext) {
    state.audioEnabled = false;
    $("#audioStatus").textContent = "Unsupported";
    return;
  }
  ensureAudioGraph(AudioContext);
  await ensureAudioWorklet();
  
  disconnectAudioSource();
  if (!state.workletLoaded) {
    state.audioEnabled = false;
    $("#audioStatus").textContent = "Worklet Error";
    return;
  }
  
  state.audioSource = state.audioContext.createMediaStreamSource(stream);
  state.impactNode = new AudioWorkletNode(state.audioContext, 'impact-processor');
  state.impactNode.port.onmessage = (event) => {
    if (event.data.type === 'hit') {
      handleAudioHit(performance.now());
    }
  };
  state.impactNode.port.postMessage({ type: 'set-sensitivity', value: state.settings.micSensitivity });
  state.audioSource.connect(state.impactNode);
  await state.audioContext.resume();
  state.audioEnabled = true;
  $("#audioStatus").textContent = "Ready";
}

async function ensureAudioWorklet() {
  if (state.audioContext && !state.workletLoaded) {
    try {
      await state.audioContext.audioWorklet.addModule('audio-processor.js');
      state.workletLoaded = true;
    } catch (e) {
      console.warn("AudioWorklet could not be loaded. Falling back to manual mode.", e);
    }
  }
}

function ensureAudioGraph(AudioContext = window.AudioContext || window.webkitAudioContext) {
  if (!state.audioContext) state.audioContext = new AudioContext();
}

function disconnectAudioSource() {
  if (state.impactNode) {
    try { state.impactNode.disconnect(); } catch (e) {}
    state.impactNode = null;
  }
  if (!state.audioSource) return;
  try {
    state.audioSource.disconnect();
  } catch (error) {
    console.debug("Audio source was already disconnected", error);
  }
  state.audioSource = null;
}

async function setupFileAudio() {
  const AudioContext = window.AudioContext || window.webkitAudioContext;
  if (!AudioContext) {
    state.audioEnabled = false;
    $("#audioStatus").textContent = "Unsupported";
    return;
  }
  ensureAudioGraph(AudioContext);
  await ensureAudioWorklet();
  
  disconnectAudioSource();
  if (!state.workletLoaded) {
    state.audioEnabled = false;
    $("#audioStatus").textContent = "Worklet Error";
    return;
  }

  if (!state.mediaElementSource) {
    state.mediaElementSource = state.audioContext.createMediaElementSource(video);
  }
  state.audioSource = state.mediaElementSource;
  
  state.impactNode = new AudioWorkletNode(state.audioContext, 'impact-processor');
  state.impactNode.port.onmessage = (event) => {
    if (event.data.type === 'hit') handleAudioHit(performance.now());
  };
  state.impactNode.port.postMessage({ type: 'set-sensitivity', value: state.settings.micSensitivity });
  
  state.audioSource.connect(state.impactNode);
  state.impactNode.connect(state.audioContext.destination); // So we can hear the file
  
  await state.audioContext.resume();
  state.audioEnabled = true;
  $("#audioStatus").textContent = "Ready";
}

function stopAudio() {
  disconnectAudioSource();
  state.audioEnabled = false;
  state.hitTimes = [];
  state.hitCount = 0;
  $("#audioStatus").textContent = "Off";
}

async function loadFile(file) {
  if (!file) return;
  stopSource();
  video.src = URL.createObjectURL(file);
  state.fileMode = true;
  video.muted = false;
  video.controls = true;
  try {
    await setupFileAudio();
    video.load();
    await new Promise((resolve, reject) => {
      if (video.readyState >= 1) {
        resolve();
        return;
      }
      video.addEventListener("loadedmetadata", resolve, { once: true });
      video.addEventListener("error", reject, { once: true });
    });
    video.currentTime = 0;
    video.pause();
    sourceReady(`Video ready · Tap Start Match`);
    showToast(`Ready: ${file.name}`);
  } catch (error) {
    console.error(error);
    $("#audioStatus").textContent = "Video audio unavailable";
    sourceReady(`Video ready · Manual scoring only`);
    showToast("Could not analyze this video's audio");
  }
}

function sourceReady(label) {
  $("#emptyState").hidden = true;
  $("#statusPill").textContent = label;
  $("#cameraBtn").disabled = true;
  $("#closeCameraBtn").disabled = false;
  $("#recordBtn").disabled = !state.stream;
  state.previous = null;
  resizeCanvas();
  if (state.animationId) {
    if (video.cancelVideoFrameCallback) video.cancelVideoFrameCallback(state.animationId);
    else cancelAnimationFrame(state.animationId);
    state.animationId = null;
  }
  if (video.requestVideoFrameCallback) {
    state.animationId = video.requestVideoFrameCallback(loop);
  } else {
    state.animationId = requestAnimationFrame(loopFallback);
  }
}

function stopSource() {
  if (state.animationId) {
    if (video.cancelVideoFrameCallback) video.cancelVideoFrameCallback(state.animationId);
    else cancelAnimationFrame(state.animationId);
    state.animationId = null;
  }
  if (state.recording) stopRecording();
  releaseWakeLock();
  setMatchRunning(false);
  stopAudio();
  state.stream?.getTracks().forEach((track) => track.stop());
  state.stream = null;
  state.fileMode = false;
  if (video.src?.startsWith("blob:")) URL.revokeObjectURL(video.src);
  video.srcObject = null;
  video.removeAttribute("src");
  video.muted = true;
  video.controls = false;
}

function closeCamera() {
  stopSource();
  video.pause();
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  $("#emptyState").hidden = false;
  $("#cameraBtn").disabled = false;
  $("#closeCameraBtn").disabled = true;
  $("#recordBtn").disabled = true;
  $("#statusPill").textContent = "Camera closed";
  $("#rallyState").textContent = "Not started";
  $("#confidence").textContent = "—";
  $("#lastSide").textContent = "—";
  showToast("Camera and match stopped");
}

async function switchCamera() {
  if (state.recording) {
    showToast("Stop recording before switching cameras");
    return;
  }
  state.facingMode = state.facingMode === "environment" ? "user" : "environment";
  $("#cameraFacingBtn").textContent = state.facingMode === "environment"
    ? "Camera: Rear"
    : "Camera: Front";

  if (!state.stream) {
    showToast(`${state.facingMode === "environment" ? "Rear" : "Front"} camera selected`);
    return;
  }

  const resumeMatch = state.matchRunning;
  const ready = await startCamera();
  if (ready && resumeMatch) {
    setMatchRunning(true);
    requestWakeLock();
    state.audioContext?.resume().catch(() => {});
    $("#statusPill").textContent = "Match active · Camera switched";
  }
}

function supportedRecordingType() {
  if (!window.MediaRecorder) return "";
  const candidates = [
    "video/mp4;codecs=h264,aac",
    "video/mp4",
    "video/webm;codecs=vp8,opus",
    "video/webm"
  ];
  if (typeof MediaRecorder.isTypeSupported !== "function") return "";
  return candidates.find((type) => MediaRecorder.isTypeSupported(type)) || "";
}

function recordingDuration() {
  const seconds = Math.max(0, Math.floor((Date.now() - state.recordingStartedAt) / 1000));
  const minutes = Math.floor(seconds / 60).toString().padStart(2, "0");
  const remainder = (seconds % 60).toString().padStart(2, "0");
  return `${minutes}:${remainder}`;
}

async function toggleRecording() {
  if (state.recording) {
    stopRecording();
    return;
  }
  if (!state.stream) {
    const ready = await startCamera();
    if (!ready) return;
  }
  if (!window.MediaRecorder) {
    showToast("Video recording is not supported by this browser");
    return;
  }

  if (state.recordingUrl) {
    URL.revokeObjectURL(state.recordingUrl);
    state.recordingUrl = null;
  }
  $("#recordingLink").classList.add("hidden");
  state.recordedChunks = [];
  const mimeType = supportedRecordingType();

  try {
    const videoStream = new MediaStream(state.stream.getVideoTracks());
    state.mediaRecorder = new MediaRecorder(
      videoStream,
      mimeType ? { mimeType, videoBitsPerSecond: 2_500_000 } : undefined
    );
  } catch (error) {
    const videoStream = new MediaStream(state.stream.getVideoTracks());
    state.mediaRecorder = new MediaRecorder(videoStream);
  }

  state.mediaRecorder.addEventListener("dataavailable", (event) => {
    if (event.data.size) state.recordedChunks.push(event.data);
  });
  state.mediaRecorder.addEventListener("stop", finalizeRecording, { once: true });
  state.mediaRecorder.start(1000);
  state.recording = true;
  requestWakeLock();
  state.recordingStartedAt = Date.now();
  $("#recordBtn").classList.add("recording");
  $("#cameraFacingBtn").disabled = true;
  state.recordingTimer = window.setInterval(() => {
    $("#recordBtn").textContent = `■ Stop ${recordingDuration()}`;
  }, 500);
  $("#recordBtn").textContent = "■ Stop 00:00";
  $("#statusPill").textContent = "Recording locally";
  showToast("Recording started");
}

function stopRecording() {
  if (!state.recording) return;
  state.recording = false;
  window.clearInterval(state.recordingTimer);
  state.recordingTimer = null;
  $("#recordBtn").classList.remove("recording");
  $("#recordBtn").textContent = "● Record Match";
  $("#cameraFacingBtn").disabled = false;
  if (state.mediaRecorder?.state !== "inactive") state.mediaRecorder.stop();
  if (!state.matchRunning) releaseWakeLock();
}

function finalizeRecording() {
  if (!state.recordedChunks.length) {
    state.mediaRecorder = null;
    $("#statusPill").textContent = "Recording failed";
    showToast("No recording data was produced");
    return;
  }
  const mimeType = state.mediaRecorder?.mimeType
    || state.recordedChunks[0]?.type
    || "video/mp4";
  const blob = new Blob(state.recordedChunks, { type: mimeType });
  state.recordingUrl = URL.createObjectURL(blob);
  const extension = mimeType.includes("webm") ? "webm" : "mp4";
  const stamp = new Date().toISOString().replace(/[:.]/g, "-");
  const link = $("#recordingLink");
  link.href = state.recordingUrl;
  link.download = `ping-pong-match-${stamp}.${extension}`;
  link.classList.remove("hidden");
  state.recordedChunks = [];
  state.mediaRecorder = null;
  $("#statusPill").textContent = "Recording ready to save";
  showToast("Recording ready · Tap Save Recording");
}

function resetRallyTracking() {
  state.previous = null;
  state.lastBall = null;
  state.detections = [];
  state.rallyActive = false;
  state.lastDetectionAt = 0;
  state.lastHitAt = 0;
  state.hitTimes = [];
  state.hitCount = 0;
}

function setMatchRunning(running) {
  state.matchRunning = running;
  if (!running) resetRallyTracking();
  $("#matchBtn").textContent = running ? "⏸ Pause Match" : "▶ Start Match";
  $("#matchBtn").classList.toggle("active", running);
  $("#rallyState").textContent = running ? "Ready" : "Not started";
  $("#rallyDot").classList.remove("live");
}

async function toggleMatch() {
  if (state.matchRunning) {
    setMatchRunning(false);
    if (state.fileMode) video.pause();
    if (!state.recording) releaseWakeLock();
    $("#statusPill").textContent = "Match paused";
    showToast("Automatic scoring paused");
    return;
  }
  if (!state.stream && !video.getAttribute("src")) {
    const ready = await startCamera();
    if (!ready) return;
  }
  resetRallyTracking();
  setMatchRunning(true);
  requestWakeLock();
  state.audioContext?.resume().catch(() => {});
  if (state.fileMode) {
    if (video.ended || video.currentTime >= video.duration - .1) video.currentTime = 0;
    await video.play().catch((error) => {
      console.error(error);
      setMatchRunning(false);
      showToast("Tap Start Match again to play the video");
    });
    if (!state.matchRunning) return;
  }
  $("#statusPill").textContent = state.audioEnabled
    ? "Match active · Waiting for rally"
    : "Match active · Manual scoring";
  showToast(state.audioEnabled
    ? "Match started · Impact detector ready"
    : "Match started · Microphone required for auto score");
}

function resizeCanvas() {
  const size = desiredCanvasSize();
  canvas.width = size.width;
  canvas.height = size.height;
}

function shouldRotateSource() {
  return window.matchMedia("(orientation: landscape)").matches
    && video.videoHeight > video.videoWidth;
}

function desiredCanvasSize() {
  const sourceWidth = video.videoWidth || 1280;
  const sourceHeight = video.videoHeight || 720;
  return shouldRotateSource()
    ? { width: sourceHeight, height: sourceWidth }
    : { width: sourceWidth, height: sourceHeight };
}

function drawVideoFrame() {
  ctx.save();
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (shouldRotateSource()) {
    ctx.translate(canvas.width, 0);
    ctx.rotate(Math.PI / 2);
    ctx.drawImage(video, 0, 0, canvas.height, canvas.width);
  } else {
    ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
  }
  ctx.restore();
}

function loop(now, metadata) {
  if (video.readyState >= 2) {
    const size = desiredCanvasSize();
    if (canvas.width !== size.width || canvas.height !== size.height) resizeCanvas();
    drawVideoFrame();
    detectBall(performance.now());
    drawOverlay();
  }
  state.animationId = video.requestVideoFrameCallback(loop);
}

function loopFallback(timestamp = 0) {
  if (video.readyState >= 2) {
    const size = desiredCanvasSize();
    if (canvas.width !== size.width || canvas.height !== size.height) resizeCanvas();
    drawVideoFrame();
    detectBall(timestamp);
    drawOverlay();
  }
  state.animationId = requestAnimationFrame(loopFallback);
}

function handleAudioHit(now) {
  if (!state.matchRunning || !state.audioEnabled) return;
  state.lastHitAt = now;
  state.hitTimes.push(now);
  state.hitTimes = state.hitTimes.filter((time) => now - time < 2600);
  state.hitCount += 1;
  $("#audioStatus").textContent = `Hits: ${state.hitCount}`;
  const rallySpan = state.hitTimes.at(-1) - state.hitTimes[0];
  if (state.hitTimes.length >= 4 && rallySpan >= 1800) {
    state.rallyActive = true;
    $("#statusPill").textContent = `Rally active · ${state.hitCount} hits`;
  }
}

function detectBall(now) {
  analysis.drawImage(canvas, 0, 0, analysisCanvas.width, analysisCanvas.height);
  const frame = analysis.getImageData(0, 0, analysisCanvas.width, analysisCanvas.height);
  if (!state.previous) {
    state.previous = frame;
    return;
  }

  const r = tableBounds(analysisCanvas.width, analysisCanvas.height);
  let best = null;
  for (let y = r.y + 1; y < r.y + r.h - 1; y += 2) {
    for (let x = r.x + 1; x < r.x + r.w - 1; x += 2) {
      if (!pointInTable(x / analysisCanvas.width, y / analysisCanvas.height)) continue;
      const i = (y * analysisCanvas.width + x) * 4;
      const r = frame.data[i];
      const g = frame.data[i + 1];
      const b = frame.data[i + 2];
      const lum = (r + g + b) / 3;
      const prevLum = (state.previous.data[i] + state.previous.data[i + 1] + state.previous.data[i + 2]) / 3;
      const motion = Math.abs(lum - prevLum);
      if (lum < 135 || motion < 42) continue;

      // HSL color filtering
      let validColor = false;
      const maxC = Math.max(r, g, b), minC = Math.min(r, g, b);
      const s = maxC === 0 ? 0 : (maxC - minC) / maxC;
      
      if (state.settings.ballColor === "white") {
        // White ball: low saturation or very bright
        validColor = s < 0.25 || lum > 200;
      } else {
        // Orange ball: High saturation, hue in orange range
        if (s > 0.35 && maxC === r) {
          const h = 60 * ((g - b) / (maxC - minC));
          validColor = (h > 15 && h < 45);
        }
      }
      
      if (!validColor) continue;

      const proximity = state.lastBall
        ? Math.max(0, 1 - Math.hypot(x - state.lastBall.x, y - state.lastBall.y) / 65)
        : .35;
      const score = motion * .65 + lum * .2 + proximity * 55;
      if (!best || score > best.score) best = { x, y, score };
    }
  }
  state.previous = frame;

  if (best) {
    state.lastBall = best;
    state.lastDetectionAt = performance.now();
    state.detections.push({ ...best, at: state.lastDetectionAt });
    state.detections = state.detections.filter((d) => state.lastDetectionAt - d.at < 2500);
    const side = tableSideAt(best.x / analysisCanvas.width, best.y / analysisCanvas.height);
    $("#lastSide").textContent = side === "left" ? "Camera left" : "Camera right";
    $("#confidence").textContent = `${Math.min(99, Math.round(best.score / 2.2))}%`;
  }

  if (state.rallyActive && now - state.lastHitAt > 1500) finishRally();

  $("#rallyState").textContent = state.matchRunning
    ? (state.rallyActive ? "Rally active" : "Ready")
    : "Not started";
  $("#rallyDot").classList.toggle("live", state.rallyActive);
}

function finishRally() {
  const recent = state.detections.slice(-5);
  if (!state.matchRunning) return;
  if (!state.auto || recent.length < 3) {
    resetRallyTracking();
    $("#statusPill").textContent = state.auto
      ? "Ball path unclear · Tap the winner"
      : "Rally ended · Auto Score is off";
    showToast(state.auto ? "Ball path unclear. Score manually." : "Rally ended");
    return;
  }
  const meanX = recent.reduce((sum, d) => sum + d.x, 0) / recent.length;
  const meanY = recent.reduce((sum, d) => sum + d.y, 0) / recent.length;
  const lastSide = tableSideAt(
    meanX / analysisCanvas.width,
    meanY / analysisCanvas.height
  );
  const candidateWinner = opposite(lastSide);
  award(candidateWinner, "Automatic estimate");
  state.lastAwardAt = performance.now();
  resetRallyTracking();
  $("#statusPill").textContent = "Match active · Waiting for rally";
  $("#audioStatus").textContent = state.audioEnabled ? "Ready" : "Manual mode";
}

function tableBounds(width, height) {
  const xs = state.tableCorners.map((point) => point.x * width);
  const ys = state.tableCorners.map((point) => point.y * height);
  const left = Math.max(0, Math.floor(Math.min(...xs)));
  const top = Math.max(0, Math.floor(Math.min(...ys)));
  const right = Math.min(width, Math.ceil(Math.max(...xs)));
  const bottom = Math.min(height, Math.ceil(Math.max(...ys)));
  return { x: left, y: top, w: right - left, h: bottom - top };
}

function pointInTable(x, y) {
  let inside = false;
  const points = state.tableCorners;
  for (let i = 0, j = points.length - 1; i < points.length; j = i++) {
    const a = points[i];
    const b = points[j];
    const intersects = (a.y > y) !== (b.y > y)
      && x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x;
    if (intersects) inside = !inside;
  }
  return inside;
}

function edgeXAtY(top, bottom, y) {
  const span = bottom.y - top.y;
  if (Math.abs(span) < .001) return (top.x + bottom.x) / 2;
  const progress = Math.max(0, Math.min(1, (y - top.y) / span));
  return top.x + (bottom.x - top.x) * progress;
}

function tableSideAt(x, y) {
  const [topLeft, topRight, bottomRight, bottomLeft] = state.tableCorners;
  const left = edgeXAtY(topLeft, bottomLeft, y);
  const right = edgeXAtY(topRight, bottomRight, y);
  return x < (left + right) / 2 ? "left" : "right";
}

function canvasTablePoints(points = state.tableCorners) {
  return points.map((point) => ({
    x: point.x * canvas.width,
    y: point.y * canvas.height
  }));
}

function drawTablePath(points) {
  if (!points.length) return;
  ctx.beginPath();
  ctx.moveTo(points[0].x, points[0].y);
  points.slice(1).forEach((point) => ctx.lineTo(point.x, point.y));
  if (points.length === 4) ctx.closePath();
}

function drawOverlay() {
  const points = canvasTablePoints();
  const topMiddle = {
    x: (points[0].x + points[1].x) / 2,
    y: (points[0].y + points[1].y) / 2
  };
  const bottomMiddle = {
    x: (points[3].x + points[2].x) / 2,
    y: (points[3].y + points[2].y) / 2
  };
  ctx.save();
  if (!state.calibrating) {
    ctx.strokeStyle = "#61e8d3";
    ctx.lineWidth = Math.max(2, canvas.width / 700);
    ctx.setLineDash([10, 8]);
    drawTablePath(points);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.beginPath();
    ctx.moveTo(topMiddle.x, topMiddle.y);
    ctx.lineTo(bottomMiddle.x, bottomMiddle.y);
    ctx.strokeStyle = "#61e8d3";
    ctx.stroke();
  } else {
    const pending = canvasTablePoints(state.calibrationPoints);
    if (pending.length > 0) {
      ctx.lineWidth = Math.max(2, canvas.width / 700);
      ctx.beginPath();
      ctx.moveTo(pending[0].x, pending[0].y);
      for (let i = 1; i < pending.length; i++) ctx.lineTo(pending[i].x, pending[i].y);
      ctx.strokeStyle = "#c7f64d";
      ctx.stroke();
    }
    pending.forEach((point, index) => {
      ctx.beginPath();
      ctx.arc(point.x, point.y, Math.max(9, canvas.width / 100), 0, Math.PI * 2);
      ctx.fillStyle = "#c7f64d";
      ctx.fill();
      ctx.fillStyle = "#142006";
      ctx.font = `bold ${Math.max(12, canvas.width / 55)}px system-ui`;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(String(index + 1), point.x, point.y);
    });
  }

  if (state.lastBall && performance.now() - state.lastDetectionAt < 350) {
    const x = state.lastBall.x / analysisCanvas.width * canvas.width;
    const y = state.lastBall.y / analysisCanvas.height * canvas.height;
    ctx.beginPath();
    ctx.arc(x, y, 12, 0, Math.PI * 2);
    ctx.strokeStyle = "#c7f64d";
    ctx.lineWidth = 4;
    ctx.stroke();
    ctx.fillStyle = "#c7f64d";
    ctx.font = `bold ${Math.max(11, canvas.width / 70)}px system-ui`;
    ctx.textAlign = "left";
    ctx.textBaseline = "bottom";
    ctx.fillText("BALL?", x + 16, y - 8);
  }
  ctx.restore();
}

function canvasPoint(event) {
  const box = canvas.getBoundingClientRect();
  return {
    x: Math.max(0, Math.min(1, (event.clientX - box.left) / box.width)),
    y: Math.max(0, Math.min(1, (event.clientY - box.top) / box.height))
  };
}

function orderTableCorners(points) {
  const center = points.reduce((result, point) => ({
    x: result.x + point.x / points.length,
    y: result.y + point.y / points.length
  }), { x: 0, y: 0 });
  const clockwise = points
    .map((point) => ({ ...point }))
    .sort((a, b) => Math.atan2(a.y - center.y, a.x - center.x)
      - Math.atan2(b.y - center.y, b.x - center.x));
  const topLeftIndex = clockwise.reduce((best, point, index) => (
    point.x + point.y < clockwise[best].x + clockwise[best].y ? index : best
  ), 0);
  return clockwise.slice(topLeftIndex).concat(clockwise.slice(0, topLeftIndex));
}

function validCalibrationShape(points) {
  if (!validTableCorners(points)) return false;
  const signedArea = points.reduce((sum, point, index) => {
    const next = points[(index + 1) % points.length];
    return sum + point.x * next.y - next.x * point.y;
  }, 0) / 2;
  const xs = points.map((point) => point.x);
  const ys = points.map((point) => point.y);
  return Math.abs(signedArea) >= .006
    && Math.max(...xs) - Math.min(...xs) >= .08
    && Math.max(...ys) - Math.min(...ys) >= .06;
}

function toggleCalibration() {
  if (!state.calibrating && video.readyState < 2) {
    showToast("Open the camera or a video first");
    return;
  }
  state.calibrating = !state.calibrating;
  state.calibrationPoints = [];
  $("#calibrateBtn").textContent = state.calibrating ? "Cancel Calibration" : "Set 4 Table Corners";
  $("#statusPill").textContent = state.calibrating
    ? "Tap any table corner · 1/4"
    : "Calibration cancelled";
  showToast(state.calibrating
    ? "Tap all four tabletop corners in any order"
    : "Calibration cancelled");
}

canvas.addEventListener("pointerdown", (event) => {
  if (!state.calibrating) return;
  event.preventDefault();
  const point = canvasPoint(event);
  const duplicate = state.calibrationPoints.some((existing) =>
    Math.hypot(existing.x - point.x, existing.y - point.y) < .025
  );
  if (duplicate) {
    showToast("That corner is already selected");
    return;
  }
  state.calibrationPoints.push(point);
  
  if (video.readyState >= 2) {
    drawVideoFrame();
    drawOverlay();
  }

  if (state.calibrationPoints.length < 4) {
    $("#statusPill").textContent = `Tap another table corner · ${state.calibrationPoints.length + 1}/4`;
    return;
  }

  const orderedCorners = orderTableCorners(state.calibrationPoints);
  if (!validCalibrationShape(orderedCorners)) {
    state.calibrationPoints = [];
    $("#statusPill").textContent = "Try again · Tap four separated corners";
    showToast("The selected table area is too small");
    if (video.readyState >= 2) {
      drawVideoFrame();
      drawOverlay();
    }
    return;
  }

  state.tableCorners = orderedCorners;
  state.calibrating = false;
  state.calibrationPoints = [];
  try {
    localStorage.setItem("pingpong-table-corners-v3", JSON.stringify(state.tableCorners));
  } catch (error) {
    console.debug("Table region could not be saved", error);
  }
  $("#calibrateBtn").textContent = "Set 4 Table Corners";
  $("#statusPill").textContent = "Four-corner table area saved";
  state.previous = null;
  showToast("Perspective table area updated");

  if (video.readyState >= 2) {
    drawVideoFrame();
    drawOverlay();
  }
});

function swapSides() {
  [state.names.left, state.names.right] = [state.names.right, state.names.left];
  [state.score.left, state.score.right] = [state.score.right, state.score.left];
  state.server = opposite(state.server);
  state.firstServer = opposite(state.firstServer);
  state.history.forEach((event) => {
    event.side = opposite(event.side);
    const oldLeft = event.before.left;
    event.before.left = event.before.right;
    event.before.right = oldLeft;
  });
  renderScore();
  showToast("Player sides swapped");
}

document.querySelectorAll("[data-score]").forEach((button) => {
  button.addEventListener("click", () => award(button.dataset.score, "Tap"));
});
$("#cameraBtn").addEventListener("click", startCamera);
$("#closeCameraBtn").addEventListener("click", closeCamera);
$("#cameraFacingBtn").addEventListener("click", switchCamera);
$("#recordBtn").addEventListener("click", toggleRecording);
$("#matchBtn").addEventListener("click", toggleMatch);
$("#fileInput").addEventListener("change", (event) => loadFile(event.target.files[0]));
$("#calibrateBtn").addEventListener("click", toggleCalibration);
$("#autoBtn").addEventListener("click", () => {
  state.auto = !state.auto;
  $("#autoBtn").classList.toggle("active", state.auto);
  $("#autoBtn").setAttribute("aria-pressed", state.auto);
  $("#autoBtn").textContent = `Auto Score: ${state.auto ? "On" : "Off"}`;
});
$("#undoBtn").addEventListener("click", undo);
$("#swapBtn").addEventListener("click", swapSides);
$("#resetBtn").addEventListener("click", reset);
$("#clearHistory").addEventListener("click", () => {
  state.history = [];
  renderHistory();
});
$("#micSensitivity").addEventListener("input", (e) => {
  const val = parseFloat(e.target.value);
  state.settings.micSensitivity = val;
  $("#sensValue").textContent = val.toFixed(1);
  if (state.impactNode) {
    state.impactNode.port.postMessage({ type: 'set-sensitivity', value: val });
  }
});
$("#ballColor").addEventListener("change", (e) => {
  state.settings.ballColor = e.target.value;
});
window.addEventListener("keydown", (event) => {
  if (event.target.matches("input")) return;
  if (event.key.toLowerCase() === "a") award("left", "Keyboard");
  if (event.key.toLowerCase() === "l") award("right", "Keyboard");
  if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === "z") undo();
});
window.addEventListener("beforeunload", (event) => {
  if (state.recording) {
    event.preventDefault();
    event.returnValue = "";
  }
  stopSource();
});
document.addEventListener("visibilitychange", () => {
  if (document.visibilityState === "visible" && (state.matchRunning || state.recording)) {
    requestWakeLock();
  }
});
window.addEventListener("orientationchange", () => {
  state.previous = null;
  if (video.readyState >= 2) resizeCanvas();
});
if ("serviceWorker" in navigator) {
  window.addEventListener("load", () => {
    navigator.serviceWorker.register("./sw.js").then((reg) => {
      reg.addEventListener("updatefound", () => {
        const newWorker = reg.installing;
        newWorker.addEventListener("statechange", () => {
          if (newWorker.state === "installed" && navigator.serviceWorker.controller) {
            const banner = $("#updateBanner");
            if (banner) {
              banner.classList.remove("hidden");
              banner.addEventListener("click", () => window.location.reload());
            }
          }
        });
      });
    }).catch((error) => {
      console.warn("Offline cache unavailable:", error);
    });
  });
}
renderScore();
