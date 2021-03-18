SRC=../../..
ORIG=$(SRC)/liblihata/regression

# list of the original lihata test documents that do not contain error
TESTS_ORIG= \
  $(ORIG)/empty.lht \
  $(ORIG)/excess_closing_braces.lht \
  $(ORIG)/hash.lht \
  $(ORIG)/list.lht \
  $(ORIG)/list_del_nth.lht \
  $(ORIG)/list_empty.lht \
  $(ORIG)/list_first_symlink.lht \
  $(ORIG)/list_missing_last_semicolon.lht \
  $(ORIG)/list_nthname.lht \
  $(ORIG)/merge_hash.lht \
  $(ORIG)/nodes_for_testing.lht \
  $(ORIG)/revpath.lht \
  $(ORIG)/stream.lht \
  $(ORIG)/sy.lht \
  $(ORIG)/symlink.lht \
  $(ORIG)/symlink_root_node.lht \
  $(ORIG)/ta.lht \
  $(ORIG)/table.lht \
  $(ORIG)/table_corners.lht \
  $(ORIG)/table_named_cell.lht \
  $(ORIG)/text.lht \
  $(ORIG)/text_root_node.lht \
  $(ORIG)/tiny.lht

test:
	@./test.sh $(TESTS_ORIG) *.lht

