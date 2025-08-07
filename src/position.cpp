#include "position.h"
#include "move_list.h"

const MoveList& Position::legalMoves() const noexcept
{
    moves_.clear(); // make sure buffer is cleared

    // push normal moves
    pushLegalMoves<MoveType::Normal, PieceType::PAWN>();
    pushLegalMoves<MoveType::Normal, PieceType::KNIGHT>();
    pushLegalMoves<MoveType::Normal, PieceType::BISHOP>();
    pushLegalMoves<MoveType::Normal, PieceType::ROOK>();
    pushLegalMoves<MoveType::Normal, PieceType::QUEEN>();
    pushLegalMoves<MoveType::Normal, PieceType::KING>();

    // push special moves
    pushLegalMoves<MoveType::DoublePawnPush, PieceType::PAWN>();
    pushLegalMoves<MoveType::Promotion, PieceType::PAWN>();
    pushLegalMoves<MoveType::Enpassant, PieceType::PAWN>();
    pushLegalMoves<MoveType::Castle, PieceType::KING>();

    return moves_;
};
