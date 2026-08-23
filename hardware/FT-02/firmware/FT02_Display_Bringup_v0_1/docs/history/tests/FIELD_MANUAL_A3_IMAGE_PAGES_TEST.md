# FieldManual Runtime A3 Image Pages Test

1. Copy `SD_ROOT/knowledge/field_manual/` to SD root.
2. Open Knowledge -> 分类浏览 -> 工具制作.
3. Open TOL001 through TOL010. Page 1 should be an image page with a Chinese caption; C advances to the first text section.
4. Z returns to the image page; B returns to the card list.
5. Temporarily remove both `media.idx` and `media.dat`, reboot, and confirm all text cards still load normally.
6. Restore media files and confirm image pages return.

Pass criteria: no BAD_FORMAT, text-only fallback works, and all ten tool cards display their image page before text.
