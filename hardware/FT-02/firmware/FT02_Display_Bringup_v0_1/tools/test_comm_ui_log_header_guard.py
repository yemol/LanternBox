from pathlib import Path
ui = Path('src/FT02_CommunicationNodeUI.cpp').read_text(encoding='utf-8')
assert 'FT02_LOGI(' in ui, 'COMM filter diagnostics unexpectedly missing'
assert '#include "FT02_Log.h"' in ui, 'FT02_CommunicationNodeUI.cpp uses FT02_LOGI without FT02_Log.h'
print('PASS COMM UI log header guard')
