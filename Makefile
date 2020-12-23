windows:
	python setup.py build_ext --inplace
linux-gnu:
	python3 setup.py build_ext --inplace
executable:
	pyinstaller --noconfirm --onefile --console "./mp_miner.py"