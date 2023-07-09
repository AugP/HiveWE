#include "unit_palette.h"

#include <map_global.h>
#include <globals.h>

#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>


#include "table_model.h"

UnitPalette::UnitPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

	for (const auto& player : map->info.players) {
		std::string color_lookup = std::to_string(player.internal_number);
		if (color_lookup.size() == 1) {
			color_lookup = "0" + color_lookup;
		}

		std::string player_name = map->trigger_strings.string(player.name) + " (" + world_edit_strings.data("WorldEditStrings", "WESTRING_UNITCOLOR_" + color_lookup) + ")";

		ui.player->addItem(QString::fromStdString(player_name), player.internal_number);
	}
	ui.player->addItem("Neutral Hostile", 24);
	ui.player->addItem("Neutral Passive", 27);

	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	selector = new UnitSelector(this);
	selector->setObjectName("selector");
	ui.verticalLayout->addWidget(selector);

	find_this = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	find_parent = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	selection_mode->setShortCut(Qt::Key_Space, { this, parent });

	current_selection_section = new QRibbonSection;
	current_selection_section->setText("Current Selection");

	QSmallRibbonButton* edit_in_oe = new QSmallRibbonButton;
	edit_in_oe->setText("Edit in OE");
	edit_in_oe->setIcon(QIcon("Data/Icons/Ribbon/objecteditor32x32.png"));
	edit_in_oe->setEnabled(false);

	QSmallRibbonButton* select_in_palette = new QSmallRibbonButton;
	select_in_palette->setText("Select in Palette");
	select_in_palette->setToolTip("Or click the doodad with middle mouse button");
	select_in_palette->setIcon(QIcon("Data/Icons/Ribbon/doodads32x32.png"));
	select_in_palette->setEnabled(false);

	QVBoxLayout* info_layout = new QVBoxLayout;
	info_layout->addWidget(selection_name);
	info_layout->addWidget(edit_in_oe);
	info_layout->addWidget(select_in_palette);

	current_selection_section->addLayout(info_layout);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(current_selection_section);
	
	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });

	connect(ui.player, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
		brush.player_id = ui.player->currentData().toInt();
	});

	connect(find_this, &QShortcut::activated, [&]() {
		activateWindow();
		selector->search->setFocus();
		selector->search->selectAll();
	});

	connect(find_parent, &QShortcut::activated, [&]() {
		activateWindow();
		selector->search->setFocus();
		selector->search->selectAll();
	});

	
	connect(selector, &UnitSelector::unitSelected, [&](const std::string& id) { 
		brush.set_unit(id); 
		selection_mode->setChecked(false);
	});

	connect(&brush, &UnitBrush::selection_changed, this, &UnitPalette::update_selection_info);
}

UnitPalette::~UnitPalette() {
	map->brush = nullptr;
}

bool UnitPalette::event(QEvent* e) {
	if (e->type() == QEvent::Close) {
		// Remove shortcut from parent
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
		selection_mode->disconnectShortcuts();
		ribbon_tab->setParent(nullptr);
		delete ribbon_tab;
	} else if (e->type() == QEvent::WindowActivate) {
		find_this->setEnabled(true);
		find_parent->setEnabled(true);
		selection_mode->enableShortcuts();
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Unit Palette");
	}
	return QWidget::event(e);
}

void UnitPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		selection_mode->disableShortcuts();
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
	}
}

void UnitPalette::update_selection_info() {
	if (brush.selections.empty()) {
		if (current_selection_section->isEnabled()) {
			current_selection_section->setEnabled(false);
		}
		selection_name->setText("");
	} else {
		if (!current_selection_section->isEnabled()) {
			current_selection_section->setEnabled(true);
		}
		const Unit& unit = **brush.selections.begin();

		bool same_object = true;
		for (const auto& i : brush.selections) {
			same_object = same_object && i->id == unit.id;
		}

		// Set the name
		if (same_object) {
			auto index = units_table->index(units_slk.row_headers.at(unit.id), units_slk.column_headers.at("name"));
			selection_name->setText(units_table->data(index).toString());
		} else {
			selection_name->setText("Various");
		}
	}
}