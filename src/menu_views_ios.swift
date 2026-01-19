/*
 * Copyright 2025 Mayur Pawashe
 * https://zgcoder.net
 
 * This file is part of skycheckers.
 * skycheckers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * skycheckers is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
 */

import SwiftUI

private let cellBackgroundColor = Color(red: 0.33, green: 0.33, blue: 0.33, opacity: 1.0)
private let cellForegroundColor = Color(red: 0.7, green: 0.7, blue: 0.7, opacity: 1.0)
private let menuCornerRadius = 8.0

// tvOS specific decorations
private extension View {
	@ViewBuilder
	func onMenuButtonBack(perform action: @escaping () -> Void) -> some View {
		#if os(tvOS)
		self.onExitCommand(perform: action)
		#else
		self
		#endif
	}

	@ViewBuilder
	func menuButtonStyle() -> some View {
		#if os(tvOS)
		self.buttonStyle(MenuButtonStyle())
		#else
		self
		#endif
	}

	@ViewBuilder
	func menuRowFocusSection() -> some View {
		#if os(tvOS)
		self.focusSection()
		#else
		self
		#endif
	}
}

#if os(tvOS)
private struct MenuButtonStyle: ButtonStyle {
	func makeBody(configuration: Configuration) -> some View {
		Inner(configuration: configuration)
	}

	private struct Inner: View {
		let configuration: ButtonStyleConfiguration
		@Environment(\.isFocused) private var isFocused

		var body: some View {
			configuration.label
				.overlay(
					RoundedRectangle(cornerRadius: menuCornerRadius)
						.fill(Color.white.opacity(isFocused ? 0.4 : 0))
				)
				.scaleEffect(isFocused ? 1.05 : 1.0)
				.animation(.easeInOut(duration: 0.15), value: isFocused)
		}
	}
}
#endif

private struct MenuStepperRow: View {
	let label: String
	let value: Int
	let range: ClosedRange<Int>
	let rowHeight: CGFloat
	let fontSize: CGFloat
	let itemWidth: CGFloat
	let onDecrement: () -> Void
	let onIncrement: () -> Void

	var body: some View {
		HStack {
			Text(label)
				.font(.system(size: fontSize))
				.foregroundColor(cellForegroundColor)
			Spacer()
			HStack(spacing: 0) {
				Button(action: onDecrement) {
					Image(systemName: "minus")
						.foregroundColor(value <= range.lowerBound ? cellForegroundColor.opacity(0.35) : cellForegroundColor)
						.frame(width: rowHeight * 0.75, height: rowHeight * 0.75)
						.background(Color(white: 0.25))
						.cornerRadius(6)
				}
				.disabled(value <= range.lowerBound)
				Divider()
					.frame(width: 1, height: rowHeight * 0.75)
					.background(cellForegroundColor.opacity(0.3))
				Button(action: onIncrement) {
					Image(systemName: "plus")
						.foregroundColor(value >= range.upperBound ? cellForegroundColor.opacity(0.35) : cellForegroundColor)
						.frame(width: rowHeight * 0.75, height: rowHeight * 0.75)
						.background(Color(white: 0.25))
						.cornerRadius(6)
				}
				.disabled(value >= range.upperBound)
			}
			.cornerRadius(6)
		}
		.padding(.horizontal, 12)
		.frame(width: itemWidth, height: rowHeight)
		.background(cellBackgroundColor)
		.cornerRadius(menuCornerRadius)
		.menuRowFocusSection()
	}
}

private struct MenuSegmentedRow: View {
	let label: String
	let segments: [String]
	@Binding var selection: Int
	let rowHeight: CGFloat
	let fontSize: CGFloat
	let itemWidth: CGFloat

	var body: some View {
		HStack {
			Text(label)
				.font(.system(size: fontSize))
				.foregroundColor(cellForegroundColor)
			Spacer()
			HStack(spacing: 0) {
				ForEach(segments.indices, id: \.self) { i in
					Button(action: { selection = i }) {
						Text(segments[i])
							.font(.system(size: fontSize * 0.7))
							.foregroundColor(selection == i ? cellBackgroundColor : cellForegroundColor)
							.frame(width: rowHeight * 1.3, height: rowHeight * 0.75)
							.background(selection == i ? cellForegroundColor : Color.clear)
					}
					if i < segments.count - 1 {
						Divider()
							.frame(width: 1, height: rowHeight * 0.75)
							.background(cellForegroundColor.opacity(0.3))
					}
				}
			}
			.background(Color(white: 0.25))
			.cornerRadius(6)
		}
		.padding(.horizontal, 12)
		.frame(width: itemWidth, height: rowHeight)
		.background(cellBackgroundColor)
		.cornerRadius(menuCornerRadius)
		.menuRowFocusSection()
	}
}

private struct MenuCancelButton: View {
	let onBack: () -> Void
	let geometry: GeometryProxy

	var body: some View {
		let buttonSize = geometry.size.width * 0.0599
		Button(action: onBack) {
			Text("✗")
				.font(.system(size: geometry.size.width * 0.032))
				.foregroundColor(cellForegroundColor)
				.frame(width: buttonSize, height: buttonSize)
				.overlay(
					RoundedRectangle(cornerRadius: 9)
						.stroke(cellForegroundColor, lineWidth: 2)
				)
		}
		.position(
			x: geometry.size.width * 0.12 + buttonSize / 2,
			y: geometry.size.height * 0.18 + buttonSize / 2
		)
	}
}

private enum MainMenuDestination {
	case online
	case options
}

private enum MainMenuItemAction {
	case navigate(MainMenuDestination)
	case play
	case tutorial
}

struct GameMenuItem: Identifiable {
	var id: String { title }
	let title: String
	fileprivate let action: MainMenuItemAction
}

struct MainMenuView: View {
	let onPlay: () -> Void
	let onTutorial: () -> Void
	let onOnlineNameChange: (String) -> Void
	let onFriendsJoiningChange: (Int) -> Void
	let onHostAddressChange: (String) -> Void
	let onStartGame: () -> Void
	let onConnect: () -> Void
	let onHumanPlayersChange: (Int) -> Void
	let onPlayerLivesChange: (Int) -> Void
	let onAIDifficultyChange: (Int) -> Void
	let onEffectsChange: (Bool) -> Void
	let onMusicChange: (Bool) -> Void
	@State private var destination: MainMenuDestination? = nil
	@State private var onlineName: String
	@State private var friendsJoining: Int
	@State private var hostAddress: String
	@State private var humanPlayers: Int
	@State private var playerLives: Int
	@State private var aiDifficulty: Int
	@State private var effectsEnabled: Bool
	@State private var musicEnabled: Bool

	init(onPlay: @escaping () -> Void, onTutorial: @escaping () -> Void,
	     initialOnlineName: String, onOnlineNameChange: @escaping (String) -> Void,
	     initialFriendsJoining: Int, onFriendsJoiningChange: @escaping (Int) -> Void,
	     initialHostAddress: String, onHostAddressChange: @escaping (String) -> Void,
	     onStartGame: @escaping () -> Void, onConnect: @escaping () -> Void,
	     initialHumanPlayers: Int, onHumanPlayersChange: @escaping (Int) -> Void,
	     initialPlayerLives: Int, onPlayerLivesChange: @escaping (Int) -> Void,
	     initialAIDifficulty: Int, onAIDifficultyChange: @escaping (Int) -> Void,
	     initialEffectsEnabled: Bool, onEffectsChange: @escaping (Bool) -> Void,
	     initialMusicEnabled: Bool, onMusicChange: @escaping (Bool) -> Void) {
		self.onPlay = onPlay
		self.onTutorial = onTutorial
		self.onOnlineNameChange = onOnlineNameChange
		self.onFriendsJoiningChange = onFriendsJoiningChange
		self.onHostAddressChange = onHostAddressChange
		self.onStartGame = onStartGame
		self.onConnect = onConnect
		self.onHumanPlayersChange = onHumanPlayersChange
		self.onPlayerLivesChange = onPlayerLivesChange
		self.onAIDifficultyChange = onAIDifficultyChange
		self.onEffectsChange = onEffectsChange
		self.onMusicChange = onMusicChange
		self._onlineName = State(initialValue: initialOnlineName)
		self._friendsJoining = State(initialValue: initialFriendsJoining)
		self._hostAddress = State(initialValue: initialHostAddress)
		self._humanPlayers = State(initialValue: initialHumanPlayers)
		self._playerLives = State(initialValue: initialPlayerLives)
		self._aiDifficulty = State(initialValue: initialAIDifficulty)
		self._effectsEnabled = State(initialValue: initialEffectsEnabled)
		self._musicEnabled = State(initialValue: initialMusicEnabled)
	}

	private let mainMenuItems = [
		GameMenuItem(title: "Play",     action: .play),
		GameMenuItem(title: "Online",   action: .navigate(.online)),
		GameMenuItem(title: "Tutorial", action: .tutorial),
		GameMenuItem(title: "Options",  action: .navigate(.options))
	]

	var body: some View {
		switch destination {
		case .online:
			OnlineMenuView(onlineName: $onlineName, onBack: { destination = nil }, initialFriendsJoining: friendsJoining, onFriendsJoiningChange: { newValue in
				friendsJoining = newValue
				onFriendsJoiningChange(newValue)
			}, initialHostAddress: hostAddress, onHostAddressChange: { newValue in
				hostAddress = newValue
				onHostAddressChange(newValue)
			}, onStartGame: onStartGame, onConnect: onConnect)
			.onChange(of: onlineName) { _, newValue in
				onOnlineNameChange(newValue)
			}
		case .options:
			OptionsMenuView(
				onBack: { destination = nil },
				initialHumanPlayers: humanPlayers, onHumanPlayersChange: { newValue in
					humanPlayers = newValue
					onHumanPlayersChange(newValue)
				},
				initialPlayerLives: playerLives, onPlayerLivesChange: { newValue in
					playerLives = newValue
					onPlayerLivesChange(newValue)
				},
				initialAIDifficulty: aiDifficulty, onAIDifficultyChange: { newValue in
					aiDifficulty = newValue
					onAIDifficultyChange(newValue)
				},
				initialEffectsEnabled: effectsEnabled, onEffectsChange: { newValue in
					effectsEnabled = newValue
					onEffectsChange(newValue)
				},
				initialMusicEnabled: musicEnabled, onMusicChange: { newValue in
					musicEnabled = newValue
					onMusicChange(newValue)
				}
			)
		case nil:
			GeometryReader { geometry in
				mainMenuContent(geometry: geometry)
			}
		}
	}

	@ViewBuilder
	private func mainMenuContent(geometry: GeometryProxy) -> some View {
		VStack {
			Spacer().frame(height: geometry.size.height * 0.3)

			ForEach(mainMenuItems) { item in
				Button(action: {
					switch item.action {
					case .navigate(let dest): destination = dest
					case .play: onPlay()
					case .tutorial: onTutorial()
					}
				}) {
					RoundedRectangle(cornerRadius: menuCornerRadius)
						.stroke(cellBackgroundColor, lineWidth: 0)
						.overlay(Text(item.title))
				}
				.font(.system(size: geometry.size.height * 0.04))
				.frame(width: geometry.size.height * 0.3, height: geometry.size.height * 0.1)
				.background(cellBackgroundColor)
				.foregroundColor(cellForegroundColor)
				.cornerRadius(menuCornerRadius)
				.padding(.bottom, geometry.size.height * 0.01)
			}

			Spacer()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
	}
}

struct OptionsMenuView: View {
	let onBack: () -> Void
	let onHumanPlayersChange: (Int) -> Void
	let onPlayerLivesChange: (Int) -> Void
	let onAIDifficultyChange: (Int) -> Void
	let onEffectsChange: (Bool) -> Void
	let onMusicChange: (Bool) -> Void
	@State private var humanPlayers: Int
	@State private var playerLives: Int
	@State private var aiDifficulty: Int
	@State private var effectsEnabled: Bool
	@State private var musicEnabled: Bool

	init(onBack: @escaping () -> Void,
		 initialHumanPlayers: Int, onHumanPlayersChange: @escaping (Int) -> Void,
		 initialPlayerLives: Int, onPlayerLivesChange: @escaping (Int) -> Void,
		 initialAIDifficulty: Int, onAIDifficultyChange: @escaping (Int) -> Void,
		 initialEffectsEnabled: Bool, onEffectsChange: @escaping (Bool) -> Void,
		 initialMusicEnabled: Bool, onMusicChange: @escaping (Bool) -> Void) {
		self.onBack = onBack
		self.onHumanPlayersChange = onHumanPlayersChange
		self.onPlayerLivesChange = onPlayerLivesChange
		self.onAIDifficultyChange = onAIDifficultyChange
		self.onEffectsChange = onEffectsChange
		self.onMusicChange = onMusicChange
		self._humanPlayers = State(initialValue: initialHumanPlayers)
		self._playerLives = State(initialValue: initialPlayerLives)
		self._aiDifficulty = State(initialValue: initialAIDifficulty)
		self._effectsEnabled = State(initialValue: initialEffectsEnabled)
		self._musicEnabled = State(initialValue: initialMusicEnabled)
	}

	var body: some View {
		GeometryReader { geometry in
			ZStack(alignment: .topLeading) {
				menuContent(geometry: geometry)
				#if !os(tvOS)
				MenuCancelButton(onBack: onBack, geometry: geometry)
				#endif
			}
		}
		.ignoresSafeArea(.keyboard)
		.onMenuButtonBack(perform: onBack)
	}

	@ViewBuilder
	private func menuContent(geometry: GeometryProxy) -> some View {
		let itemWidth = geometry.size.height * 0.65
		let rowHeight = geometry.size.height * 0.1
		let itemSpacing = geometry.size.height * 0.01
		let fontSize = geometry.size.height * 0.04
		let headerFontSize = geometry.size.height * 0.03

		VStack {
			Spacer().frame(height: geometry.size.height * 0.3)

			Text("Gameplay")
				.font(.system(size: headerFontSize, weight: .bold))
				.foregroundColor(Color(white: 0.5))
				.frame(width: itemWidth, alignment: .leading)

			MenuSegmentedRow(
				label: "Bot Difficulty",
				segments: ["Easy", "Medium", "Hard"],
				selection: $aiDifficulty,
				rowHeight: rowHeight, fontSize: fontSize, itemWidth: itemWidth
			)
			.onChange(of: aiDifficulty) { _, newValue in onAIDifficultyChange(newValue) }
			.padding(.bottom, itemSpacing)

			MenuStepperRow(
				label: humanPlayers == 1 ? "1 Human Player" : "\(humanPlayers) Human Players",
				value: humanPlayers, range: 0...4,
				rowHeight: rowHeight, fontSize: fontSize, itemWidth: itemWidth,
				onDecrement: { if humanPlayers > 0 { humanPlayers -= 1; onHumanPlayersChange(humanPlayers) } },
				onIncrement: { if humanPlayers < 4 { humanPlayers += 1; onHumanPlayersChange(humanPlayers) } }
			)
			.padding(.bottom, itemSpacing)

			MenuStepperRow(
				label: playerLives == 1 ? "1 Player Life" : "\(playerLives) Player Lives",
				value: playerLives, range: 1...10,
				rowHeight: rowHeight, fontSize: fontSize, itemWidth: itemWidth,
				onDecrement: { if playerLives > 1 { playerLives -= 1; onPlayerLivesChange(playerLives) } },
				onIncrement: { if playerLives < 10 { playerLives += 1; onPlayerLivesChange(playerLives) } }
			)
			.padding(.bottom, itemSpacing)

			Text("Audio")
				.font(.system(size: headerFontSize, weight: .bold))
				.foregroundColor(Color(white: 0.5))
				.frame(width: itemWidth, alignment: .leading)

			let toggleScale = rowHeight / 62

			HStack {
				Text("Effects")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
				Spacer()
				Toggle("", isOn: $effectsEnabled)
					.labelsHidden()
					.tint(Color(red: 0.2, green: 0.6, blue: 0.2))
					.scaleEffect(toggleScale)
					.frame(width: 51 * toggleScale, height: 31 * toggleScale)
					.padding(.trailing, 8)
					.onChange(of: effectsEnabled) { _, newValue in onEffectsChange(newValue) }
			}
			.padding(.horizontal, 12)
			.frame(width: itemWidth, height: rowHeight)
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.menuRowFocusSection()
			.padding(.bottom, itemSpacing)

			HStack {
				Text("Music")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
				Spacer()
				Toggle("", isOn: $musicEnabled)
					.labelsHidden()
					.tint(Color(red: 0.2, green: 0.6, blue: 0.2))
					.scaleEffect(toggleScale)
					.frame(width: 51 * toggleScale, height: 31 * toggleScale)
					.padding(.trailing, 8)
					.onChange(of: musicEnabled) { _, newValue in onMusicChange(newValue) }
			}
			.padding(.horizontal, 12)
			.frame(width: itemWidth, height: rowHeight)
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.menuRowFocusSection()
			.padding(.bottom, itemSpacing)

			Spacer()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
	}
}

struct MakeGameMenuView: View {
	let onBack: () -> Void
	let onFriendsJoiningChange: (Int) -> Void
	let onStartGame: () -> Void
	@State private var friendsJoining: Int

	init(onBack: @escaping () -> Void, initialFriendsJoining: Int, onFriendsJoiningChange: @escaping (Int) -> Void, onStartGame: @escaping () -> Void) {
		self.onBack = onBack
		self.onFriendsJoiningChange = onFriendsJoiningChange
		self.onStartGame = onStartGame
		self._friendsJoining = State(initialValue: initialFriendsJoining)
	}

	var body: some View {
		GeometryReader { geometry in
			ZStack(alignment: .topLeading) {
				menuContent(geometry: geometry)
				#if !os(tvOS)
				MenuCancelButton(onBack: onBack, geometry: geometry)
				#endif
			}
		}
		.ignoresSafeArea(.keyboard)
		.onMenuButtonBack(perform: onBack)
	}

	@ViewBuilder
	private func menuContent(geometry: GeometryProxy) -> some View {
		let itemWidth = geometry.size.height * 0.55
		let rowHeight = geometry.size.height * 0.1
		let itemSpacing = geometry.size.height * 0.01
		let fontSize = geometry.size.height * 0.04

		VStack {
			Spacer().frame(height: geometry.size.height * 0.3)

			MenuStepperRow(
				label: friendsJoining == 1 ? "1 Friend" : "\(friendsJoining) Friends",
				value: friendsJoining, range: 1...3,
				rowHeight: rowHeight, fontSize: fontSize, itemWidth: itemWidth,
				onDecrement: {
					if friendsJoining > 1 {
						friendsJoining -= 1
						onFriendsJoiningChange(friendsJoining)
					}
				},
				onIncrement: {
					if friendsJoining < 3 {
						friendsJoining += 1
						onFriendsJoiningChange(friendsJoining)
					}
				}
			)
			.padding(.bottom, itemSpacing)

			Button(action: { onStartGame() }) {
				Text("Start Game")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
					.frame(width: itemWidth, height: rowHeight)
			}
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Spacer()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
	}
}

struct JoinGameMenuView: View {
	let onBack: () -> Void
	let onHostAddressChange: (String) -> Void
	let onConnect: () -> Void
	@State private var hostAddress: String
	@FocusState private var addressFieldFocused: Bool

	init(onBack: @escaping () -> Void, initialHostAddress: String, onHostAddressChange: @escaping (String) -> Void, onConnect: @escaping () -> Void) {
		self.onBack = onBack
		self.onHostAddressChange = onHostAddressChange
		self.onConnect = onConnect
		self._hostAddress = State(initialValue: initialHostAddress)
	}

	var body: some View {
		GeometryReader { geometry in
			ZStack(alignment: .topLeading) {
				menuContent(geometry: geometry)
				#if !os(tvOS)
				if !addressFieldFocused {
					MenuCancelButton(onBack: onBack, geometry: geometry)
				}
				#endif
			}
		}
		.ignoresSafeArea(.keyboard)
		.onChange(of: hostAddress) { _, newValue in
			onHostAddressChange(newValue)
		}
		.onMenuButtonBack(perform: onBack)
	}

	@ViewBuilder
	private func menuContent(geometry: GeometryProxy) -> some View {
		let itemWidth = geometry.size.height * 0.7
		let rowHeight = geometry.size.height * 0.1
		let itemSpacing = geometry.size.height * 0.01
		let fontSize = geometry.size.height * 0.04

		VStack {
			Spacer().frame(height: geometry.size.height * 0.3)

			HStack {
				Text("Host Address:")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
				Spacer()
				TextField("", text: $hostAddress, prompt: Text("address").foregroundColor(cellForegroundColor.opacity(0.5)))
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
					.multilineTextAlignment(.trailing)
					.submitLabel(.done)
					.focused($addressFieldFocused)
					#if os(tvOS)
					.keyboardType(.URL)
					#else
					.keyboardType(.numbersAndPunctuation)
					#endif
					.autocorrectionDisabled()
					.textInputAutocapitalization(.never)
					.frame(width: itemWidth * 0.45)
			}
			.padding(.horizontal, 12)
			.frame(width: itemWidth, height: rowHeight)
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Button(action: { onConnect() }) {
				Text("Connect")
					.font(.system(size: fontSize))
					.foregroundColor(hostAddress.isEmpty ? cellForegroundColor.opacity(0.35) : cellForegroundColor)
					.frame(width: itemWidth, height: rowHeight)
			}
			.disabled(hostAddress.isEmpty)
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Spacer()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
	}
}

struct OnlineMenuView: View {
	@Binding var onlineName: String
	private enum Destination { case makeGame, joinGame }

	let onBack: () -> Void
	let onFriendsJoiningChange: (Int) -> Void
	let onHostAddressChange: (String) -> Void
	let onStartGame: () -> Void
	let onConnect: () -> Void
	@FocusState private var nameFieldFocused: Bool
	@State private var destination: Destination? = nil
	@State private var friendsJoining: Int
	@State private var hostAddress: String

	init(onlineName: Binding<String>, onBack: @escaping () -> Void, initialFriendsJoining: Int, onFriendsJoiningChange: @escaping (Int) -> Void, initialHostAddress: String, onHostAddressChange: @escaping (String) -> Void, onStartGame: @escaping () -> Void, onConnect: @escaping () -> Void) {
		self._onlineName = onlineName
		self.onBack = onBack
		self.onFriendsJoiningChange = onFriendsJoiningChange
		self.onHostAddressChange = onHostAddressChange
		self.onStartGame = onStartGame
		self.onConnect = onConnect
		self._friendsJoining = State(initialValue: initialFriendsJoining)
		self._hostAddress = State(initialValue: initialHostAddress)
	}

	var body: some View {
		switch destination {
		case .makeGame:
			MakeGameMenuView(onBack: { destination = nil }, initialFriendsJoining: friendsJoining, onFriendsJoiningChange: { newValue in
				friendsJoining = newValue
				onFriendsJoiningChange(newValue)
			}, onStartGame: onStartGame)
		case .joinGame:
			JoinGameMenuView(onBack: { destination = nil }, initialHostAddress: hostAddress, onHostAddressChange: { newValue in
				hostAddress = newValue
				onHostAddressChange(newValue)
			}, onConnect: onConnect)
		case nil:
			GeometryReader { geometry in
				ZStack(alignment: .topLeading) {
					menuContent(geometry: geometry)
					#if !os(tvOS)
					if !nameFieldFocused {
						MenuCancelButton(onBack: onBack, geometry: geometry)
					}
					#endif
				}
			}
			.ignoresSafeArea(.keyboard)
			.onMenuButtonBack(perform: onBack)
		}
	}

	@ViewBuilder
	private func menuContent(geometry: GeometryProxy) -> some View {
		let itemWidth = geometry.size.height * 0.42
		let rowHeight = geometry.size.height * 0.1
		let itemSpacing = geometry.size.height * 0.01
		let fontSize = geometry.size.height * 0.04

		VStack {
			Spacer().frame(height: geometry.size.height * 0.3)

			HStack {
				Text("Name:")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
				Spacer()
				TextField("", text: $onlineName, prompt: Text("Name").foregroundColor(cellForegroundColor.opacity(0.5)))
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
					.multilineTextAlignment(.trailing)
					.submitLabel(.done)
					.focused($nameFieldFocused)
					.frame(width: itemWidth * 0.45)
			}
			.padding(.horizontal, 12)
			.frame(width: itemWidth, height: rowHeight)
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Button(action: { destination = .makeGame }) {
				Text("Make Game")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
					.frame(width: itemWidth, height: rowHeight)
			}
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Button(action: { destination = .joinGame }) {
				Text("Join Game")
					.font(.system(size: fontSize))
					.foregroundColor(cellForegroundColor)
					.frame(width: itemWidth, height: rowHeight)
			}
			.background(cellBackgroundColor)
			.cornerRadius(menuCornerRadius)
			.padding(.bottom, itemSpacing)

			Spacer()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
	}
}

@objc class MainMenuController: NSObject {
	@MainActor @objc(viewControllerWithOnPlay:onTutorial:initialOnlineName:onOnlineNameChange:initialFriendsJoining:onFriendsJoiningChange:initialHostAddress:onHostAddressChange:onStartGame:onConnect:initialHumanPlayers:onHumanPlayersChange:initialPlayerLives:onPlayerLivesChange:initialAIDifficulty:onAIDifficultyChange:initialEffectsEnabled:onEffectsChange:initialMusicEnabled:onMusicChange:)
	static func viewController(onPlay: @escaping () -> Void,
							   onTutorial: @escaping () -> Void,
							   initialOnlineName: String,
							   onOnlineNameChange: @escaping (String) -> Void,
							   initialFriendsJoining: Int,
							   onFriendsJoiningChange: @escaping (Int) -> Void,
							   initialHostAddress: String,
							   onHostAddressChange: @escaping (String) -> Void,
							   onStartGame: @escaping () -> Void,
							   onConnect: @escaping () -> Void,
							   initialHumanPlayers: Int,
							   onHumanPlayersChange: @escaping (Int) -> Void,
							   initialPlayerLives: Int,
							   onPlayerLivesChange: @escaping (Int) -> Void,
							   initialAIDifficulty: Int,
							   onAIDifficultyChange: @escaping (Int) -> Void,
							   initialEffectsEnabled: Bool,
							   onEffectsChange: @escaping (Bool) -> Void,
							   initialMusicEnabled: Bool,
							   onMusicChange: @escaping (Bool) -> Void) -> UIViewController {
		return UIHostingController(rootView: MainMenuView(onPlay: onPlay, onTutorial: onTutorial, initialOnlineName: initialOnlineName, onOnlineNameChange: onOnlineNameChange, initialFriendsJoining: initialFriendsJoining, onFriendsJoiningChange: onFriendsJoiningChange, initialHostAddress: initialHostAddress, onHostAddressChange: onHostAddressChange, onStartGame: onStartGame, onConnect: onConnect, initialHumanPlayers: initialHumanPlayers, onHumanPlayersChange: onHumanPlayersChange, initialPlayerLives: initialPlayerLives, onPlayerLivesChange: onPlayerLivesChange, initialAIDifficulty: initialAIDifficulty, onAIDifficultyChange: onAIDifficultyChange, initialEffectsEnabled: initialEffectsEnabled, onEffectsChange: onEffectsChange, initialMusicEnabled: initialMusicEnabled, onMusicChange: onMusicChange))
	}
}

struct PauseMenuView: View {
	let onResume: () -> Void
	let onExit: () -> Void

	var body: some View {
		GeometryReader { geometry in
			let itemWidth = geometry.size.height * 0.3
			let rowHeight = geometry.size.height * 0.1
			let itemSpacing = geometry.size.height * 0.01
			let fontSize = geometry.size.height * 0.04

			VStack {
				Spacer().frame(height: geometry.size.height * 0.35)

				Button(action: onResume) {
					Text("Resume")
						.font(.system(size: fontSize))
						.foregroundColor(cellForegroundColor)
						.frame(width: itemWidth, height: rowHeight)
				}
				.background(cellBackgroundColor)
				.cornerRadius(menuCornerRadius)
				.padding(.bottom, itemSpacing)

				Button(action: onExit) {
					Text("Exit")
						.font(.system(size: fontSize))
						.foregroundColor(cellForegroundColor)
						.frame(width: itemWidth, height: rowHeight)
				}
				.background(cellBackgroundColor)
				.cornerRadius(menuCornerRadius)
				.padding(.bottom, itemSpacing)

				Spacer()
			}
			.frame(maxWidth: .infinity, maxHeight: .infinity)
		.menuButtonStyle()
		}
		.onMenuButtonBack(perform: onResume)
	}
}

@objc class PauseMenuController: NSObject {
	@MainActor @objc(viewControllerWithOnResume:onExit:)
	static func viewController(onResume: @escaping () -> Void, onExit: @escaping () -> Void) -> UIViewController {
		return UIHostingController(rootView: PauseMenuView(onResume: onResume, onExit: onExit))
	}
}
