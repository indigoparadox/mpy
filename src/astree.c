
#include <stdlib.h>

#include "maug.h"

#define ASTREE_C
#include "astree.h"

int16_t astree_init( struct ASTREE* tree ) {
   int16_t retval = 0;

   memset( tree, '\0', sizeof( struct ASTREE ) );

   tree->nodes_sz = 1;
   tree->nodes = calloc( tree->nodes_sz, sizeof( struct ASTREE_NODE ) );
   if( NULL == tree->nodes ) {
      error_printf( "could not allocate syntax tree!" );
      retval = -1;
      goto cleanup;
   }

   astree_node_initialize( tree, 0, -1 );
   astree_node( tree, 0 )->type = ASTREE_NODE_TYPE_SEQUENCE;

cleanup:
   return retval;
}

void astree_free( struct ASTREE* tree ) {
   free( tree->nodes );
   tree->nodes_sz = 0;
}

int16_t astree_node_find_free( struct ASTREE* tree ) {
   uint16_t i = 0;
   struct ASTREE_NODE* new_nodes = NULL;

   for( i = 0 ; tree->nodes_sz > i ; i++ ) {
      if( !(tree->nodes[i].active) ) {
         return i;
      }
   }

   /* Could not find free node! */
   assert( tree->nodes_sz * 2 > tree->nodes_sz );
   new_nodes = realloc( tree->nodes,
      (tree->nodes_sz * 2) * sizeof( struct ASTREE_NODE ) );
   assert( NULL != new_nodes );
   tree->nodes = new_nodes;
   for( i = tree->nodes_sz ; tree->nodes_sz * 2 > i ; i++ ) {
      /* Make sure new nodes are inactive. */
      memset( &(tree->nodes[i]), '\0', sizeof( struct ASTREE_NODE ) );
   }
   tree->nodes_sz *= 2;

   /* Now that there are more nodes, try again! */
   return astree_node_find_free( tree );
}

void astree_node_initialize(
   struct ASTREE* tree, int16_t node_idx, int16_t parent_idx
) {
   tree->nodes[node_idx].parent = parent_idx;
   tree->nodes[node_idx].active = 1;
   tree->nodes[node_idx].first_child = -1;
   tree->nodes[node_idx].next_sibling = -1;
   tree->nodes[node_idx].prev_sibling = -1;
}

int16_t astree_node_insert_child_parent(
   struct ASTREE* tree, int16_t parent_idx
) {
   int16_t node_idx_out = -1,
      prev_child_idx = -1;

   /* Grab the current child list. */
   prev_child_idx = tree->nodes[parent_idx].first_child;
   tree->nodes[parent_idx].first_child = -1;

   /* Replace with the new child and tack the list on to that. */
   node_idx_out = astree_node_add_child( tree, parent_idx );
   if( 0 > node_idx_out ) {
      /* Problem! Put it back! */
      tree->nodes[parent_idx].first_child = prev_child_idx;
      goto cleanup;
   }

   debug_printf( 1, "inserting %d between %d and %d...",
      node_idx_out, parent_idx, prev_child_idx );

   tree->nodes[parent_idx].first_child = node_idx_out;
   tree->nodes[node_idx_out].first_child = prev_child_idx;
   tree->nodes[prev_child_idx].parent = node_idx_out;

cleanup:

   return node_idx_out;
}

int16_t astree_node_add_child( struct ASTREE* tree, int16_t parent_idx ) {
   int16_t iter = -1,
      node_out = -1;

   /* Grab a free node to append. */
   node_out = astree_node_find_free( tree );
   if( 0 > node_out ) {
      goto cleanup;
   }

   /* Figure out where to append it. */
   if( 0 > tree->nodes[parent_idx].first_child ) {
      /* Create the first child. */
      tree->nodes[parent_idx].first_child = node_out;
   } else {
      for(
         iter = tree->nodes[parent_idx].first_child ;
         0 <= tree->nodes[iter].next_sibling ;
         iter = tree->nodes[iter].next_sibling
      ) {}

      /* Create the new sibling of the last child. */
      tree->nodes[iter].next_sibling = node_out;
      assert( -1 != iter );
   }

   astree_node_initialize( tree, node_out, parent_idx );

   tree->nodes[node_out].prev_sibling = iter;

cleanup:

   return node_out;
}

void astree_dump( struct ASTREE* tree, int16_t node_idx, int16_t depth ) {

   if( 0 == depth ) {
      debug_printf( ASTREE_DUMP_DEBUG_LVL, "--- TREE MAP ---" );
   }

   switch( tree->nodes[node_idx].type ) {
   case ASTREE_NODE_TYPE_SEQUENCE:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) sequence node", depth, node_idx );
      break;

   case ASTREE_NODE_TYPE_FUNC_DEF:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) function def node: %s", depth, node_idx,
         tree->nodes[node_idx].value.s );
      break;

   case ASTREE_NODE_TYPE_FUNC_CALL:
      debug_printf( ASTREE_DUMP_DEBUG_LVL, 
         "\t%d: (idx: %d) function call node: %s", depth, node_idx,
         tree->nodes[node_idx].value.s );
      break;

   case ASTREE_NODE_TYPE_IF:
      debug_printf( ASTREE_DUMP_DEBUG_LVL, 
         "\t%d: (idx: %d) if node", depth, node_idx );
      break;

   case ASTREE_NODE_TYPE_LITERAL:
      switch( tree->nodes[node_idx].value_type ) {
      case ASTREE_VALUE_TYPE_NONE:
         debug_printf( ASTREE_DUMP_DEBUG_LVL, 
            "\t%d: (idx: %d) literal node: none", depth, node_idx );
         break;

      case ASTREE_VALUE_TYPE_INT:
         debug_printf( ASTREE_DUMP_DEBUG_LVL, 
            "\t%d: (idx: %d) literal node: %d", depth, node_idx,
            tree->nodes[node_idx].value.i );
         break;

      case ASTREE_VALUE_TYPE_FLOAT:
         debug_printf( ASTREE_DUMP_DEBUG_LVL,
            "\t%d: (idx: %d) literal node: %f", depth, node_idx,
            tree->nodes[node_idx].value.f );
         break;

      case ASTREE_VALUE_TYPE_STRING:
         debug_printf( ASTREE_DUMP_DEBUG_LVL, "\t%d: (idx: %d) literal node: \"%s\"",
            depth, node_idx, tree->nodes[node_idx].value.s );
         break;
      }
      break;

   case ASTREE_NODE_TYPE_COND:
      switch( tree->nodes[node_idx].value_type ) {
      case ASTREE_VALUE_TYPE_GT:
         debug_printf( ASTREE_DUMP_DEBUG_LVL,
            "\t%d: (idx: %d) cond node: greater than", depth, node_idx );
         break;

      case ASTREE_VALUE_TYPE_EQ:
         debug_printf( ASTREE_DUMP_DEBUG_LVL,
            "\t%d: (idx: %d) cond node: equal to", depth, node_idx );
         break;
      }
      break;

   case ASTREE_NODE_TYPE_OP:
      switch( tree->nodes[node_idx].value_type ) {
      case ASTREE_VALUE_TYPE_ADD:
         debug_printf( ASTREE_DUMP_DEBUG_LVL,
            "\t%d: (idx: %d) op node: add", depth, node_idx );
         break;
      }
      break;

   case ASTREE_NODE_TYPE_VARIABLE:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) variable node: %s", depth, node_idx,
         tree->nodes[node_idx].value.s );
      break;

   case ASTREE_NODE_TYPE_ASSIGN:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) assign node", depth, node_idx );
      break;

   case ASTREE_NODE_TYPE_FUNC_DEF_PARM:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) function def parm node: %s",
         depth, node_idx, tree->nodes[node_idx].value.s );
      break;

#if 0
   case ASTREE_NODE_TYPE_FUNC_CALL_PARMS:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) function call parms node",
         depth, node_idx );
      break;
#endif

   default:
      debug_printf( ASTREE_DUMP_DEBUG_LVL,
         "\t%d: (idx: %d) unknown node", depth, node_idx );
      break;
   }

   debug_printf( 0, "first_child: %d, next_sibling: %d, prev_sibling: %d",
      tree->nodes[node_idx].first_child,
      tree->nodes[node_idx].next_sibling,
      tree->nodes[node_idx].prev_sibling );

   if( 0 <= tree->nodes[node_idx].first_child ) {
      astree_dump( tree, tree->nodes[node_idx].first_child, depth + 1 );
   }

   if( 0 <= tree->nodes[node_idx].next_sibling ) {
      astree_dump( tree, tree->nodes[node_idx].next_sibling, depth );
   }

   if( 0 == depth ) {
      debug_printf( ASTREE_DUMP_DEBUG_LVL, "--- END TREE MAP ---" );
   }
}

