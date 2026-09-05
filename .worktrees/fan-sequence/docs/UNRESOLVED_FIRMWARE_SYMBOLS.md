# Unresolved firmware symbols

The evidence manifest retains 165 numbered symbols because the current code, TI reference release, and fixed-address layout do not establish a unique semantic name.
These names are intentionally left unchanged rather than replaced with plausible-looking guesses.

## Summary

| Category | Count |
| --- | ---: |
| `branch_target` | 45 |
| `state` | 114 |
| `storage` | 6 |

## Evidence policy

A retained symbol may be renamed later when a matching vendor declaration, an exact hardware-register definition, a uniquely identifiable structure field, or a decisive cross-reference becomes available. Address proximity or a single assignment is not sufficient evidence.

## Retained symbols

| Symbol | Address | Category | First observed use | Reason retained |
| --- | --- | --- | --- | --- |
| `core_branch_target_003` | `0x08000D3C` | `branch_target` | `src/rdx_core.c:74` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_004` | `0x08000D40` | `branch_target` | `src/rdx_core.c:77` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_007` | `0x08000E7A` | `branch_target` | `src/rdx_core.c:247` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_011` | `0x08001092` | `branch_target` | `src/rdx_core.c:424` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_012` | `0x08001106` | `branch_target` | `src/rdx_core.c:419` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_013` | `0x08001432` | `branch_target` | `src/rdx_core.c:816` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_015` | `0x0800152A` | `branch_target` | `src/rdx_core.c:751` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_017` | `0x0800157C` | `branch_target` | `src/rdx_core.c:722` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_018` | `0x080015AE` | `branch_target` | `src/rdx_core.c:794` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_028` | `0x08001B78` | `branch_target` | `src/rdx_core.c:1301` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_029` | `0x08001B80` | `branch_target` | `src/rdx_core.c:1336` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `ahci_branch_target_001` | `0x08001D08` | `branch_target` | `src/ahci.c:54` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `ahci_branch_target_002` | `0x08001D0A` | `branch_target` | `src/ahci.c:64` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_039` | `0x08002868` | `branch_target` | `src/rdx_core.c:1991` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_040` | `0x080028D8` | `branch_target` | `src/rdx_core.c:1985` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_041` | `0x080028DA` | `branch_target` | `src/rdx_core.c:1982` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_042` | `0x080028E0` | `branch_target` | `src/rdx_core.c:2021` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_044` | `0x08002B2C` | `branch_target` | `src/rdx_core.c:2141` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_045` | `0x08002B30` | `branch_target` | `src/rdx_core.c:2143` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_046` | `0x08002B38` | `branch_target` | `src/rdx_core.c:2148` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_049` | `0x08002CD2` | `branch_target` | `src/rdx_core.c:2297` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_050` | `0x08002CE2` | `branch_target` | `src/rdx_core.c:2258` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_051` | `0x08002F0E` | `branch_target` | `src/rdx_core.c:2574` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_052` | `0x08002F42` | `branch_target` | `src/rdx_core.c:2535` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_056` | `0x08003980` | `branch_target` | `src/rdx_core.c:3073` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_058` | `0x080039B8` | `branch_target` | `src/rdx_core.c:3047` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_059` | `0x08003C08` | `branch_target` | `src/rdx_core.c:3321` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_061` | `0x08003C1C` | `branch_target` | `src/rdx_core.c:3297` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_062` | `0x08003C20` | `branch_target` | `src/rdx_core.c:3285` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_063` | `0x08003C8C` | `branch_target` | `src/rdx_core.c:3282` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_073` | `0x0800492C` | `branch_target` | `src/rdx_core.c:4133` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_074` | `0x0800492E` | `branch_target` | `src/rdx_core.c:4127` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_081` | `0x08004CE8` | `branch_target` | `src/rdx_core.c:4343` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_082` | `0x08004CEA` | `branch_target` | `src/rdx_core.c:4335` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_086` | `0x08004FA0` | `branch_target` | `src/rdx_core.c:4465` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_087` | `0x08004FA8` | `branch_target` | `src/rdx_core.c:4480` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_002` | `0x08004FB8` | `state` | `src/rdx_core.c:4491` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_100` | `0x08006B50` | `branch_target` | `src/rdx_core.c:6312` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_101` | `0x08006B58` | `branch_target` | `src/rdx_core.c:6322` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_105` | `0x0800746C` | `branch_target` | `src/rdx_core.c:7024` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_106` | `0x0800747A` | `branch_target` | `src/rdx_core.c:7029` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_107` | `0x08007484` | `branch_target` | `src/rdx_core.c:7018` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_117` | `0x0800D460` | `branch_target` | `src/rdx_core.c:8512` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_008` | `0x0800E398` | `state` | `src/rdx_core.c:9450` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_012` | `0x0800E679` | `state` | `src/rdx_core.c:3680` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_002` | `0x0800E67A` | `storage` | `src/rdx_core.c:3682` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_003` | `0x0800E67B` | `storage` | `src/rdx_core.c:3682` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_118` | `0x0800E6B4` | `branch_target` | `src/rdx_core.c:11128` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_119` | `0x0800E6B6` | `branch_target` | `src/rdx_core.c:11132` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_branch_target_120` | `0x0800E760` | `branch_target` | `src/rdx_core.c:1717` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_013` | `0x0800E77C` | `state` | `src/rdx_core.c:1713` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_014` | `0x0800E78C` | `state` | `src/rdx_core.c:1718` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_015` | `0x0800E79C` | `state` | `src/rdx_core.c:1724` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_016` | `0x0800E7AC` | `state` | `src/rdx_core.c:1725` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_017` | `0x0800E7BC` | `state` | `src/rdx_core.c:1726` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_019` | `0x0800E84F` | `state` | `src/rdx_core.c:6227` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_020` | `0x0800E852` | `state` | `src/rdx_core.c:6226` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_021` | `0x0800E855` | `state` | `src/rdx_core.c:6228` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_023` | `0x0800E85E` | `state` | `src/rdx_core.c:5644` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_029` | `0x0800E948` | `state` | `src/rdx_core.c:7873` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_032` | `0x0800E99C` | `state` | `src/rdx_core.c:6663` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_033` | `0x0800E9BC` | `state` | `src/rdx_core.c:407` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_034` | `0x0800E9BD` | `state` | `src/rdx_core.c:344` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_035` | `0x0800E9C0` | `state` | `src/rdx_core.c:426` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_036` | `0x0800E9C4` | `state` | `src/rdx_core.c:464` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_037` | `0x0800E9C5` | `state` | `src/rdx_core.c:330` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_038` | `0x0800E9C8` | `state` | `src/rdx_core.c:456` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_039` | `0x0800E9CC` | `state` | `src/rdx_core.c:401` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_040` | `0x0800E9CD` | `state` | `src/rdx_core.c:321` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_041` | `0x0800E9D8` | `state` | `src/rdx_core.c:160` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_044` | `0x0800EA0C` | `state` | `src/rdx_core.c:12951` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_050` | `0x0800EA75` | `state` | `src/mww.c:200` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_051` | `0x0800EA76` | `state` | `src/rdx_core.c:11290` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_052` | `0x0800EA7C` | `state` | `src/rdx_core.c:11282` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_055` | `0x0800EA8C` | `state` | `src/rdx_core.c:2850` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_056` | `0x0800EA8F` | `state` | `src/rdx_core.c:6909` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_057` | `0x0800EA90` | `state` | `src/rdx_core.c:2853` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_058` | `0x0800EA92` | `state` | `src/rdx_core.c:10327` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_059` | `0x0800EA94` | `state` | `src/rdx_core.c:2823` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_060` | `0x0800EA95` | `state` | `src/rdx_core.c:8009` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_061` | `0x0800EA96` | `state` | `src/rdx_core.c:8010` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_062` | `0x0800EA97` | `state` | `src/rdx_core.c:6900` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_063` | `0x0800EA98` | `state` | `src/rdx_core.c:2852` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_064` | `0x0800EA99` | `state` | `src/rdx_core.c:6906` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_067` | `0x0800EAC4` | `state` | `src/rdx_core.c:3982` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_068` | `0x0800EB3C` | `state` | `src/rdx_core.c:6597` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_069` | `0x0800EB54` | `state` | `src/rdx_core.c:15658` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_070` | `0x0800EB58` | `state` | `src/rdx_core.c:5469` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_071` | `0x0800EB64` | `state` | `src/rdx_core.c:3982` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_003` | `0x0800EC18` | `state` | `src/mww.c:816` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_074` | `0x0800EC4C` | `state` | `src/rdx_core.c:2971` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_004` | `0x0800EC54` | `state` | `src/mww.c:819` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_078` | `0x0800ECA8` | `state` | `src/rdx_core.c:1727` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_080` | `0x0800ECB6` | `state` | `src/rdx_core.c:1714` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_081` | `0x0800ECBE` | `state` | `src/rdx_core.c:1713` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_082` | `0x0800ECD0` | `state` | `src/rdx_core.c:1721` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_084` | `0x0800ECD8` | `state` | `src/rdx_core.c:2726` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_085` | `0x0800ED1A` | `state` | `src/rdx_core.c:1720` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_086` | `0x0800ED1C` | `state` | `src/rdx_core.c:1722` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_088` | `0x0800ED26` | `state` | `src/rdx_core.c:1718` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_089` | `0x0800ED38` | `state` | `src/rdx_core.c:2719` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_090` | `0x0800ED3C` | `state` | `src/rdx_core.c:2720` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_091` | `0x0800ED40` | `state` | `src/rdx_core.c:1716` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_093` | `0x0800ED4A` | `state` | `src/rdx_core.c:1724` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_094` | `0x0800ED5A` | `state` | `src/rdx_core.c:1725` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_095` | `0x0800ED6A` | `state` | `src/rdx_core.c:1726` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_099` | `0x0800EDA7` | `state` | `src/rdx_core.c:2064` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_102` | `0x0800EDD4` | `state` | `src/rdx_core.c:1974` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_103` | `0x0800EDD5` | `state` | `src/rdx_core.c:1938` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_104` | `0x0800EDD6` | `state` | `src/rdx_core.c:1940` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_105` | `0x0800EDD7` | `state` | `src/rdx_core.c:1945` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_108` | `0x0800EDE0` | `state` | `src/rdx_core.c:1973` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_110` | `0x0800EDE8` | `state` | `src/rdx_core.c:2183` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_111` | `0x0800EDEC` | `state` | `src/rdx_core.c:2129` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_113` | `0x0800EDF4` | `state` | `src/rdx_core.c:3280` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_114` | `0x0800EDF5` | `state` | `src/rdx_core.c:2202` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_115` | `0x0800EDF6` | `state` | `src/rdx_core.c:2197` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_116` | `0x0800EDF7` | `state` | `src/rdx_core.c:2199` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_119` | `0x0800EDFA` | `state` | `src/rdx_core.c:3291` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_121` | `0x0800EDFD` | `state` | `src/rdx_core.c:4911` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_122` | `0x0800EDFE` | `state` | `src/rdx_core.c:4914` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_127` | `0x0800EE1B` | `state` | `src/rdx_core.c:5941` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_128` | `0x0800EE1C` | `state` | `src/rdx_core.c:5945` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_130` | `0x0800EE1F` | `state` | `src/rdx_core.c:2091` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_133` | `0x0800EE22` | `state` | `src/rdx_core.c:2097` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_148` | `0x0800EE54` | `state` | `src/rdx_core.c:5553` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_150` | `0x0800EE5C` | `state` | `src/rdx_core.c:5547` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_153` | `0x0800EE70` | `state` | `src/rdx_core.c:467` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_156` | `0x0800EE73` | `state` | `src/rdx_core.c:5085` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_217` | `0x0800F28C` | `state` | `src/rdx_core.c:3070` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_218` | `0x0800F2D0` | `state` | `src/rdx_core.c:12018` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_219` | `0x0800F418` | `state` | `src/rdx_core.c:10906` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `interrupt_state_004` | `0x0800F41C` | `state` | `src/ahci.c:416` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_220` | `0x0800F428` | `state` | `src/rdx_core.c:7202` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `interrupt_state_005` | `0x0800F42C` | `state` | `src/vim_intvecs.c:39` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `interrupt_state_006` | `0x0800F430` | `state` | `src/vim_intvecs.c:59` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `interrupt_state_007` | `0x0800F434` | `state` | `src/vim_intvecs.c:41` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_179` | `0x0800F485` | `state` | `src/ahci.c:80` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_180` | `0x0800F487` | `state` | `src/ahci.c:135` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_181` | `0x0800F48A` | `state` | `src/rdx_core.c:602` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_221` | `0x0800F494` | `state` | `src/rdx_core.c:6836` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_182` | `0x0800F4A7` | `state` | `src/ahci.c:403` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_005` | `0x0800F4B0` | `state` | `src/mww.c:54` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_183` | `0x0800F4B4` | `state` | `src/rdx_core.c:624` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_184` | `0x0800F4C0` | `state` | `src/rdx_core.c:671` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_185` | `0x0800F4C8` | `state` | `src/ahci.c:362` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_006` | `0x0800F61C` | `state` | `src/mww.c:298` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_007` | `0x0800F61D` | `state` | `src/mww.c:295` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_012` | `0x0800F628` | `state` | `src/mww.c:268` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_013` | `0x0800F62C` | `state` | `src/mww.c:267` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_226` | `0x0800F644` | `state` | `src/rdx_core.c:14788` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_020` | `0x0800F650` | `state` | `src/mww.c:300` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `mww_state_021` | `0x0800F654` | `state` | `src/mww.c:301` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_188` | `0x0800F65A` | `state` | `src/rdx_core.c:10859` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_190` | `0x0800F65D` | `state` | `src/rdx_core.c:4567` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `scsi_state_001` | `0x0800F674` | `state` | `src/scsi.c:27` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `scsi_state_002` | `0x0800F684` | `state` | `src/scsi.c:28` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_195` | `0x0800F694` | `state` | `src/rdx_core.c:15680` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_198` | `0x0800F6B4` | `state` | `src/rdx_core.c:13647` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_230` | `0x0800F6C8` | `state` | `src/rdx_core.c:12100` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_004` | `0xC0000004` | `storage` | `src/rdx_core.c:6468` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_005` | `0xC0000008` | `storage` | `src/rdx_core.c:6472` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_006` | `0xC000000C` | `storage` | `src/rdx_core.c:6473` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_state_213` | `0xFB000134` | `state` | `src/ahci.c:406` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
| `core_storage_013` | `0xFFFFFFFF` | `storage` | `src/rdx_core.c:2545` | Current reads, writes, and call sites do not establish a unique semantic role beyond the numbered recovered storage or control-flow slot. |
