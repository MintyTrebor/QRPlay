const char *HTML_CONTENT_HOME = R"=====(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="EN" lang="EN">
  <head>
    <title>QRPLay</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no" />
    <script type="text/javascript" src="qrcode.js"></script>
  </head>
  <body>
	<h1>QR Play</h1>
	<form>
		<h2>Settings</h2>
		<table width="80%">
			<tr>
				<td width="20%"><label>MiSTer IP: </label></td>
				<td width="80%"><input id="misterIP" type="text" name="misterIP" value="mister.local" style="width:80%" /></td>
			</tr>
			<tr>
				<td width="20%"><label>Return To Menu on Remove: </label></td>
				<td width="80%">
					<select id="selResetOnRem" name="selResetOnRem">
						<option value="true">Yes</option>
						<option value="false">No</option>
					</select>
				</td>
			</tr>
			<tr>
				<td width="20%"></td>
				<td width="80%"><input type="submit" value="Save"> (ESP32-CAM will reset on save)</td>
			</tr>
		</table>		
	</form>
	<br>
	<br>
	<br>
	<div>
		<h2>QR Code Generator</h2>
		<h3>Search for Game</h3>
		System: <select id="selSys" name="selSys">
			<option>*</option>
		</select>
		Game: <input id="searchGame" type="text" name="searchGame" value="" style="width:50%" />
		<input id="btnSearch" type="button" value="Search" onclick="runSearch();">
		<input id="btnIndx" type="button" value="Update DB Index" onclick="mediaIndexSystems();">
	</div>
	<p>
	<div id="indexingDiv"></div>
	<div id="searchResults" name="searchResults"></div>	
	<div id="qrcode" style="width:125px; height:125px; margin-top:15px;"></div>
	<br>
	<br>
	<h4>QRPlay Monitor</h4>
	<div id="logDiv" name="logDiv" style="width:80%; max-height:200px; margin-top:15px; overflow:auto; display: flex; flex-direction: column-reverse;"></div><br>
	
	
	<script type="text/javascript">
		
		var qrcode = new QRCode(document.getElementById("qrcode"), {
			width : 125,
			height : 125,
			useSVG: false
		});

		var gateway = `ws://${window.location.hostname}/ws`;
		var websocket;
		window.addEventListener('load', onload);

		function makeCode () {		
			var elText = document.getElementById("resSel").value;
			console.log("QRCode Data", elText)
			if (elText == "null") {
				alert("Select a Game");
				elText.focus();
				return;
			}

			qrcode.makeCode(elText);
		}

		function runSearch() {
			
			var sGame = document.getElementById("searchGame").value;
			var sSys = this.getSelectedSystemsValue();

			document.getElementById("searchResults").innerHTML = `<h4>Searching....</h4>`;
			

			if(sSys == "*"){sSys = ""};
			if(sGame){
				
				let socket = new WebSocket("ws://mister.local:7497/")

				socket.onopen = function(e) {
					console.log("[open] Connection established 1");
					let wscmd = {
						jsonrpc: "2.0",
						id: "47f80537-7a5d-11ef-9c7b-020304050607",
						method: "media.search",
						params: {
							query: sGame,
							maxResults: 250
						}	
					};
					if(sSys != ""){wscmd.params.systems = [sSys]};
					socket.send(JSON.stringify(wscmd));
				};

				socket.onmessage = function(event) {
					//console.log(`[message] Data received from server: ${event.data}`);
					var strGames = event.data;;
					var objGames = JSON.parse(strGames);
					//console.log("objGames", objGames.result)

					if(objGames.result.total > 0) {
					
						var resultsDiv = document.getElementById("searchResults");

						document.getElementById("searchResults").innerHTML = "";
						
						var resultsLabel = document.createElement("h3");
						if(objGames.result.total > 250) {
							resultsLabel.innerText = "Search Results (" + objGames.result.total + ") - Max 250 shown - Please refine your search!";
						}
						else{
							resultsLabel.innerText = "Search Results (" + objGames.result.total + ")";
						}
						resultsDiv.appendChild(resultsLabel);
						
						var resultsLabel = document.createElement("label");
						resultsLabel.innerText = "Select a Game to generate the QR Code :";
						resultsDiv.appendChild(resultsLabel);
						
						var resultsSelect = document.createElement("select");
						resultsSelect.id = "resSel";
						resultsSelect.name = "resSel";
						resultsDiv.appendChild(resultsSelect);

						let topel = document.createElement("option");
						topel.textContent = "Choose Game";
						topel.value = "null";
						document.getElementById("resSel").appendChild(topel);

						document.getElementById("resSel").addEventListener("change", makeCode, false);
						
						var sysItem = 0;
						for(sysItem in objGames.result.results){
							let optionTxt = objGames.result.results[sysItem].system.name + ": " + objGames.result.results[sysItem].name;
							let optionVal = objGames.result.results[sysItem].path;
							let el = document.createElement("option");
							el.textContent = optionTxt;
							el.value = optionVal;
							document.getElementById("resSel").appendChild(el);
						}
					}
					else{
						var resultsDiv = document.getElementById("searchResults");
						var resultsLabel = document.createElement("h2");
						resultsLabel.innerText = "Search Results (0)";
						resultsDiv.appendChild(resultsLabel);
					}
					socket.close(1000, "Work complete");
					qrcode.clear();
					document.getElementById("qrcode").innerHTML = "";
					qrcode = new QRCode(document.getElementById("qrcode"), {
						width : 125,
						height : 125,
						useSVG: false
					});
					
				};

				socket.onclose = function(event) {
					if (event.wasClean) {
						console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
					} else {
						console.log('[close] Connection died');
					}
				};

				socket.onerror = function(error) {
					console.log(`[error]`);
				};
			}
			else{
				alert("Input Search Text");
			}
		}

		function getIndexedSystems() {
			
			let socket = new WebSocket("ws://mister.local:7497/")

			socket.onopen = function(e) {
				console.log("[open] Connection established 1");
				let wscmd = {
					jsonrpc: "2.0",
    				id: "dbd312f3-7a5f-11ef-8f29-020304050607",
    				method: "systems"
				};
				socket.send(JSON.stringify(wscmd));
			};

			socket.onmessage = function(event) {
				//console.log(`[message] Data received from server: ${event.data}`);
				var strSystems = event.data;
				var objSystems = JSON.parse(strSystems);
				//console.log("objSystems", objSystems.result)
				let select = document.getElementById("selSys");
				var sysItem = 0;
				
				objSystems.result.systems.sort(function (a, b) {
					if (a.name < b.name) {
						return -1;
					}
					if (a.name > b.name) {
						return 1;
					}
					return 0;
				});

				for(sysItem in objSystems.result.systems){
					let optionTxt = objSystems.result.systems[sysItem].name;
					let optionVal = objSystems.result.systems[sysItem].id;
					let el = document.createElement("option");
					el.textContent = optionTxt;
					el.value = optionVal;
					select.appendChild(el);
				}

				var input = document.getElementById("searchGame");
				input.addEventListener("keyup", function(event) {
					if (event.key === "Enter") {
						event.preventDefault();
						document.getElementById("btnSearch").click();
					}
				});
				socket.close(1000, "Work complete");
			};

			socket.onclose = function(event) {
				if (event.wasClean) {
					console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
				} else {
					console.log('[close] Connection died');
				}
			};

			socket.onerror = function(error) {
				console.log(`[error]`);
			};
			
		}

		function mediaIndexSystems() {
			let socket = new WebSocket("ws://mister.local:7497/")

			socket.onopen = function(e) {
				console.log("[open] Connection established");
				let wscmd = {
					jsonrpc: "2.0",
					id: "6f20e07c-7a5e-11ef-84bb-020304050607",
					method: "media.index"
				};
				socket.send(JSON.stringify(wscmd));
			};

			socket.onmessage = function(event) {
				//console.log(`[message] Data received from server: ${event.data}`);
				var strIndex = event.data;
				var objIndex = JSON.parse(strIndex);
				console.log("objIndex", objIndex)
				if(objIndex.params.indexing) {
					document.getElementById("indexingDiv").innerHTML = `<h4>Indexing - Total To Index: ${objIndex.params.totalSteps}  -  Currently Processing: ${objIndex.params.currentStep}</h4>`;
				}else{
					document.getElementById("indexingDiv").innerHTML = `<h4>Indexing Complete</h4>`;
					socket.close(1000, "Work complete");
					this.getIndexedSystems();
				}
			};

			socket.onclose = function(event) {
				if (event.wasClean) {
					console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
				} else {
					console.log('[close] Connection died');
				}
			};

			socket.onerror = function(error) {
				console.log(`[error]`);
			};

		}

		//Get Index Systems and populate selection box
		function getSysForSelect() {
            
			var strSystems = this.getIndexedSystems();
			console.log("strSystems", strSystems)
			var objSystems = JSON.parse(strSystems);
			let select = document.getElementById("selSys");
			var sysItem = 0;

			for(sysItem in objSystems.systems){
				let optionTxt = objSystems.systems[sysItem].name;
				let optionVal = objSystems.systems[sysItem].id;
				let el = document.createElement("option");
                el.textContent = optionTxt;
                el.value = optionVal;
                select.appendChild(el);
			}

        }
		

		function getSelectedSystemsValue() {
			var select = document.getElementById("selSys");
			var selectedValue = select.value;
			return selectedValue;
		}

		function onload(event) {
			initWebSocket();
			getIndexedSystems();
		}

		function addLogLine(message){
			var logger = document.getElementById('logDiv');
			if (typeof message == 'object') {
            	logger.innerHTML += (JSON && JSON.stringify ? JSON.stringify(message) : message) + '<br />';
			} else {
				logger.innerHTML += message + '<br />';
			}
		}

		function initWebSocket() {
			console.log('Trying to open a WebSocket connection…');
			websocket = new WebSocket(gateway);
			websocket.onopen    = onOpen;
			websocket.onclose   = onClose;
			websocket.onmessage = onMessage;
		}

		function onOpen(event) {
			addLogLine('Connection to QRPlay opened');
		}

		function onClose(event) {
			addLogLine('Connection to QRPLay closed');
			setTimeout(initWebSocket, 2000);
		}
		function onMessage(event) {
			addLogLine(event.data);
			if(event.data == "closeWS"){
				websocket.close();
			}
		}
				
	</script>
  </body>
</html>
)=====";